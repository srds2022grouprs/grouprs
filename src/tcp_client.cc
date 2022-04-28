#include "tcp_client.hh"
#include <cassert>
using namespace std::chrono;

TCPClient::~TCPClient() = default;

void TCPClient::connect_one(sockpp::inet_address sock_addr, uint16_t index) {
  sockpp::tcp_connector &cur_conn = node_conns[index];
  while (!(cur_conn = sockpp::tcp_connector(sock_addr))) {
    std::this_thread::sleep_for(milliseconds(1));
  }
  cur_conn.write_n(&node_index, sizeof(node_index));
}

TCPClient::TCPClient(uint16_t node_i, std::vector<socket_address> &sa, unsigned long node_size, uint zone_num, uint node_num, uint rs_k, uint rs_m, uint thr_num)
    : ThreadPool<RepairInfo>(thr_num),
      node_index(node_i),
      node_size_byte(node_size * 1024 * 1024),
      zone_num(zone_num),
      node_num(node_num),
      rs_k(rs_k),
      rs_m(rs_m),
      conn_num{static_cast<uint16_t>(sa.size())},
      node_conns(new sockpp::tcp_connector[sa.size() + 2]),
      req_mtxs{new std::mutex[sa.size() + 2]} {
  sockpp::socket_initializer sockInit;
  std::thread connect_thr[sa.size() + 2];
  uint16_t j = 0;
  for (auto &addr : sa) {
    if (j == node_index) {
      ++j;
    }
    connect_thr[j] = std::thread(&TCPClient::connect_one, this,
                                 sockpp::inet_address{addr.ip, addr.port}, j);
    ++j;
  }
  for (j = 0; j <= conn_num; ++j) {
    if (j == node_index) {
      continue;
    } else {
      connect_thr[j].join();
    }
  }
}

void TCPClient::run() {
  if (node_index == 0) {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);
    uint copy_from_other_data_center = 0;
    uint flag_copy_from_other_data_center;
    uint task_round_num = 0;   
    uint task_no = 0;
    while (true) {
      flag_copy_from_other_data_center = false;
      std::vector<RepairInfo> task_package;
      while (true) {
        RepairInfo task = get_task();
        task_package.emplace_back(task);
        if (task.task_index == task.task_num_of_cur_round) {
          break;
        }
      }
      if (!task_package[0].last_task_flag) {
        for (auto & task : task_package) {
          if (task.target.size() > task.m) {
            copy_from_other_data_center++;
            flag_copy_from_other_data_center = true;
            break;
          }
        }
        if (flag_copy_from_other_data_center) {
          continue;
        } else {
          task_round_num++;
        }
      }
      for (auto & task : task_package) {
        for (auto host : task.source) {
          std::unique_lock<std::mutex> lck = std::unique_lock<std::mutex>(req_mtxs[host]);
          sockpp::tcp_connector *cur_conn = &node_conns[host];
          task.isHelper = true;
          if (cur_conn->write_n(&task, sizeof(task)) == -1) {
            exit(-1);
          }
          for (auto num : task.source) {
            if (cur_conn->write_n(&num, sizeof(num)) == -1) {
              exit(-1);
            }
          }
          for (auto num : task.target) {
            if (cur_conn->write_n(&num, sizeof(num)) == -1) {
              exit(-1);
            }
          }
          lck.unlock();
        }
        for (auto host : task.target) {
          sockpp::tcp_connector *cur_conn;
          std::unique_lock<std::mutex> lck;
          lck = std::unique_lock<std::mutex>(req_mtxs[host]);
          cur_conn = &node_conns[host];
          task.isHelper = false;
          if (cur_conn->write_n(&task, sizeof(task)) == -1) {
            exit(-1);
          }
          for (auto num : task.source) {
            if (cur_conn->write_n(&num, sizeof(num)) == -1) {
              exit(-1);
            }
          }
          for (auto num : task.target) {
            if (cur_conn->write_n(&num, sizeof(num)) == -1) {
              exit(-1);
            }
          }
          lck.unlock();
        }
      }
      if (task_package[0].last_task_flag) {
        gettimeofday(&end_time, nullptr);
        double fin_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                      (end_time.tv_usec - start_time.tv_usec) / 1000;
        sleep(1);
        std::cout << fin_time;
        break;
      }
      std::unique_lock<std::mutex> task_lck(task_mutex);
      task_cv.wait(task_lck, [&]{return task_finish_flag;});
      task_finish_flag = false;
      task_lck.unlock();
    }
  } else {
    while (true) {
      RepairInfo task = get_task();
      if (task.last_task_flag) {
        break;
      }
      if (task.isHelper == false) {
        assert(task.finish_flag == true);
        std::unique_lock<std::mutex> lck(req_mtxs[0]);
        bool finish_signal = true;
        auto ret = node_conns[0].write_n(&finish_signal, sizeof(finish_signal));
        if (ret == -1) {
          exit(-1);
        } else {
        }
        lck.unlock();
      } else {
        for (auto host : task.target) {
          std::unique_lock<std::mutex> lck(req_mtxs[host]);
          char *buf = new char[node_size_byte]();
          FILE *f = fopen(task.read_fn, "r");
          if (!f) {
            exit(-1);
          }
          ssize_t n, i = 0, remain = node_size_byte;
          while (!feof(f) && (n = fread(buf + i, 1, remain, f)) > 0) {
            if (node_conns[host].write_n(buf + i, n) == -1) {
              exit(-1);
            }
            i += n;
            remain -= n;
          }
          fclose(f);
          delete[] buf;
          lck.unlock();
        }
      }
    }
  }
  node_conns[node_index].close();
}

void TCPClient::start_client() {
  std::thread run_thread([&]{this->run();});
  run_thread.join();
}
