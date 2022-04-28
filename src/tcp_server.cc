#include "tcp_server.hh"

#include <utility>
#include <algorithm>
#include <cassert>
#include <exception>

TCPServer::~TCPServer() { acc.close(); }

TCPServer::TCPServer(uint _node_index, in_port_t _port, uint n, uint8_t k, uint8_t m, unsigned long node_size, uint zone_num)
    : ThreadPool<RepairInfo>(1),
      self_index(_node_index),
      port(_port),
      serving_num(n),
      serving_flag(false),
      blocks_num(0),
      finished_num(0),
      rs_k(k),
      rs_m(m),
      node_size_byte(node_size * 1024 * 1024), 
      mem_pool(2 * n, node_size * 1024 * 1024),
      socks(new sockpp::tcp_socket[n + 1]),
      thr{new std::thread[n]} {
  sockpp::socket_initializer sockInit;
  acc = sockpp::tcp_acceptor(port);

  if (!acc) {
    std::cerr << "Error creating the acceptor: " << acc.last_error_str()
              << std::endl;
    exit(-1);
  }
}

void TCPServer::run(uint16_t client_index) {
  std::unique_lock<std::mutex> lck(serving_mtx);
  while (!serving_flag) {
    serving_cv.wait(lck, [&] { return serving_flag; });
  }
  lck.unlock();

  if (self_index == 0) {
    while (true) {
      if (client_index == 1) {
        std::vector<RepairInfo> task_package;
        bool flag_copy_from_other_data_center = false;
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
              flag_copy_from_other_data_center = true;
              break;
            }
          }
          if (flag_copy_from_other_data_center) continue;
        }
        requestor_index_vec.clear();
        for (auto task : task_package) {
          for (auto index : task.target) {
            requestor_index_vec.push_back(index);
          }
        }
        std::unique_lock<std::mutex> cur_task_finish_lck(cur_task_finish_mtx);
        cur_task_finish_flag = false;
        cur_task_finish_lck.unlock();
        std::unique_lock<std::mutex> read_requestor_index_lck(read_requestor_index_mtx);
        read_requestor_index_flag = true;
        if (task_package[0].last_task_flag) {
          serving_finish_flag_2 = true;
        }
        read_requestor_index_lck.unlock();
        read_requestor_index_cv.notify_all();
        if (task_package[0].last_task_flag) {
          break;
        }
        std::unique_lock<std::mutex> received_finish_signal_num_lck;
        if (find(requestor_index_vec.begin(), requestor_index_vec.end(), client_index) != requestor_index_vec.end()) {
          bool finish_signal = false;
          if (socks[client_index].read_n(&finish_signal, sizeof(finish_signal)) <= 0) {
            exit(-1);
          }
          received_finish_signal_num_lck = std::unique_lock<std::mutex>(received_finish_signal_num_mtx);
          received_finish_signal_num++;
          received_finish_signal_num_lck.unlock();
        } 
        received_finish_signal_num_lck = std::unique_lock<std::mutex>(received_finish_signal_num_mtx);
        received_finish_signal_num_cv.wait(received_finish_signal_num_lck, [&]{return received_finish_signal_num == requestor_index_vec.size();});
        received_finish_signal_num = 0;
        received_finish_signal_num_lck.unlock();
        read_requestor_index_flag = false;
        cur_task_finish_lck = std::unique_lock<std::mutex>(cur_task_finish_mtx);
        cur_task_finish_flag = true;
        cur_task_finish_lck.unlock();
        cur_task_finish_cv.notify_all();
        std::unique_lock<std::mutex> finish_thread_lck(finish_thread_mutex);
        finish_thread_cv.wait(finish_thread_lck, [&]{return finish_thread_num == serving_num - 1;});
        finish_thread_num = 0;
        finish_thread_lck.unlock();
        std::unique_lock<std::mutex> task_lck(task_mutex);
        task_finish_flag = true;
        task_lck.unlock();
        task_cv.notify_all();
      } else {
        std::unique_lock<std::mutex> read_requestor_index_lck(read_requestor_index_mtx);
        read_requestor_index_cv.wait(read_requestor_index_lck, [&]{return read_requestor_index_flag || serving_finish_flag_2;});
        read_requestor_index_lck.unlock();
        if (serving_finish_flag_2 == true) {
          break;
        }
        if (find(requestor_index_vec.begin(), requestor_index_vec.end(), client_index) != requestor_index_vec.end()) {
          bool finish_signal = false;
          if (socks[client_index].read_n(&finish_signal, sizeof(finish_signal)) <= 0) {
            exit(-1);
          }
          std::unique_lock<std::mutex> received_finish_signal_num_lck(received_finish_signal_num_mtx);
          received_finish_signal_num++;
          received_finish_signal_num_lck.unlock();
          received_finish_signal_num_cv.notify_all();
        } 
        std::unique_lock<std::mutex> cur_task_finish_lck(cur_task_finish_mtx);
        cur_task_finish_cv.wait(cur_task_finish_lck, [&]{return cur_task_finish_flag == true;});
        cur_task_finish_lck.unlock();
        std::unique_lock<std::mutex> finish_thread_lck(finish_thread_mutex);
        finish_thread_num++;
        finish_thread_lck.unlock();
        finish_thread_cv.notify_all();
      }
    }
  } else {
    if (client_index != 0) {
      while (true) {
        std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
        start_to_receive_data_cv.wait(receive_data_lck, [&]{return (start_to_receive_data_flag || serving_finish_flag);});
        receive_data_lck.unlock();  
        if (cur_task.last_task_flag) {
          break;
        }
        if (serving_finish_flag) {
          break;
        }
        if (find(cur_task.source.begin(), cur_task.source.end(), client_index) != cur_task.source.end()) {
          char* buffer = mem_pool.get_block();
          ssize_t n, i = 0, remain = node_size_byte;
          while (remain && (n = socks[client_index].read_n(buffer + i, remain)) > 0) {
            i += n;
            remain -= n;
          }
          if (i == 0) {
            exit(-1);
          }
          std::unique_lock<std::mutex> receive_data_finish_lck(receive_data_finish_mtx);
          received_data_num++;
          data_buffer.push_back(buffer);
          receive_data_finish_lck.unlock();
          receive_data_finish_cv.notify_all();
        }
        receive_data_lck.lock();
        start_to_receive_data_cv.wait(receive_data_lck, [&]{return start_to_receive_data_flag == false;});
        receive_data_lck.unlock();
        std::unique_lock<std::mutex> finish_thread_lck_2(finish_thread_mutex_2);
        finish_thread_num_2++;
        finish_thread_lck_2.unlock();
        finish_thread_cv_2.notify_all();
      }
    } else {
      uint task_no = 0;
      while (!serving_finish_flag) {
        RepairInfo task;
        auto temp = socks[0].read_n(&task, sizeof(task));
        if (temp <= 0) {
          std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
          serving_finish_flag = true;
          receive_data_lck.unlock();
          start_to_receive_data_cv.notify_all();
          exit(-1);
        }
        std::vector<uint> temp_vec;
        memcpy(&task.source, &temp_vec, sizeof(task.source));
        memcpy(&task.target, &temp_vec, sizeof(task.target));
        cur_task = task;
        cur_task.source.clear();
        cur_task.target.clear();
        uint source_size = cur_task.source_size, target_size = cur_task.target_size, num;
        for (uint i = 0; i < source_size; i++) {
          if (socks[0].read_n(&num, sizeof(num)) <= 0) {
            std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
            serving_finish_flag = true;
            receive_data_lck.unlock();
            start_to_receive_data_cv.notify_all();
            exit(-1);
          }
          cur_task.source.push_back(num);
        }
        for (uint i = 0; i < target_size; i++) {
          if (socks[0].read_n(&num, sizeof(num)) <= 0) {
            std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
            serving_finish_flag = true;
            receive_data_lck.unlock();
            start_to_receive_data_cv.notify_all();
            exit(-1);
          }
          cur_task.target.push_back(num);
        }
        if (cur_task.last_task_flag) {
          add_task(cur_task);
          std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
          start_to_receive_data_flag = true;
          receive_data_lck.unlock();
          start_to_receive_data_cv.notify_all();
          break;
        }
        if (cur_task.isHelper) {
          add_task(cur_task);
        } else {
          std::unique_lock<std::mutex> receive_data_lck(start_to_receive_data_mtx);
          start_to_receive_data_flag = true;
          receive_data_lck.unlock();
          start_to_receive_data_cv.notify_all();
          uint source_size = cur_task.source_size;
          std::unique_lock<std::mutex> receive_data_finish_lck(receive_data_finish_mtx);
          receive_data_finish_cv.wait(receive_data_finish_lck, [&]{return received_data_num == source_size;});
          received_data_num = 0;
          receive_data_finish_lck.unlock();
          receive_data_lck.lock();
          start_to_receive_data_flag = false;
          receive_data_lck.unlock();
          start_to_receive_data_cv.notify_all();
          for (char* ptr : data_buffer) {
            mem_pool.release_block(ptr);
          }
          data_buffer.clear();
          RepairInfo finish_signal;
          finish_signal.finish_flag = true;
          finish_signal.isHelper = false;
          add_task(finish_signal);
          std::unique_lock<std::mutex> finish_thread_lck_2(finish_thread_mutex_2);
          finish_thread_cv_2.wait(finish_thread_lck_2, [&]{return finish_thread_num_2 == serving_num - 1;});
          finish_thread_num_2 = 0;
          finish_thread_lck_2.unlock();
        }
      }
    }
  }
  socks[client_index].close();
}

void TCPServer::wait_for_connection() {
  uint16_t i = 0;
  uint16_t client_index;
  while (i < serving_num) {
    sockpp::inet_address peer;
    sockpp::tcp_socket sock = acc.accept(&peer);
    if (!sock) {
      std::cerr << "Error accepting incoming connection: "
                << acc.last_error_str() << std::endl;
    } else {
      sock.read_n(&client_index, sizeof(client_index));
      socks[client_index] = std::move(sock);
      thr[i++] = std::thread(&TCPServer::run, this, client_index);
    }
  }
}

void TCPServer::start_serving() {
  std::unique_lock<std::mutex> lck(serving_mtx);
  serving_flag = true;
  lck.unlock();
  serving_cv.notify_all();

  for (uint16_t i = 0; i < serving_num; ++i) {
    thr[i].join();
  }
}