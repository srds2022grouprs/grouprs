#include "tcp_node.hh"
#include "repair_info.hh"
#include "task_generator.hh"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cassert>
#include "utils.hh"
#include <ctime>

std::vector<std::vector<uint>> TCPNode::fd_vec;
std::vector<std::vector<uint>> TCPNode::zone_vec;

TCPNode::TCPNode(uint16_t index)
    : self_index(index),
      server{nullptr},
      client{nullptr} {
  std::string fn("config/nodes_config.ini");
  node_num = parse_config(fn);
  uint data_node_num = node_num - 1;
  if (index == 0) {
    uint fd_num;
    if (data_node_num % failure_domain_size == 0) {
      fd_num = data_node_num / failure_domain_size;
    } else {
      fd_num = data_node_num / failure_domain_size + 1;
    }
    if (zone_num >= failure_domain_size) {
      for (auto i = 1; i <= data_node_num; ) {
        std::vector<uint> cur_fd;
        while (cur_fd.size() < failure_domain_size && i <= data_node_num) {
          cur_fd.push_back(i++);
        }
        fd_vec.push_back(cur_fd);
      }
      assert(fd_vec.size() == fd_num);
    } else {
      std::vector<uint> shuffle_vec;
      for (uint i = 1; i <= data_node_num; i++) {
        shuffle_vec.push_back(i);
      }
      uint seed = std::time(0);
      std::shuffle(shuffle_vec.begin(), shuffle_vec.end(), std::default_random_engine(seed));
      std::vector<uint> cur_fd;
      for (auto item : shuffle_vec) {
        cur_fd.push_back(item);
        if (cur_fd.size() == failure_domain_size) {
          fd_vec.push_back(cur_fd);
          cur_fd.clear();
        }
      }
      if (!cur_fd.empty()) {
        fd_vec.push_back(cur_fd);
      }
    }
  }
}

uint16_t TCPNode::parse_config(const std::string &fn) {
  std::fstream config(fn.c_str(), std::ios_base::in);
  if (!config.is_open()) {
    exit(-1);
  }
  uint16_t total_node_num = 0;
  std::string ip;
  in_port_t port;
  if (!config.eof()) {
    config >> rs_k >> rs_m >> zone_num >> node_size >> node_lifetime >> failure_domain_size >> sim_duration;
    N = 2 * rs_k + rs_m;
  } else {
    exit(-1);
  }
  if (rs_k <= rs_m) {
    exit(-1);
  }
  if (rs_k <= rs_m) {
    exit(-1);
  }
  while (!config.eof()) {
    config >> ip >> port;
    if (config.fail()) {
      break;
    }
    if (total_node_num != self_index) {
      targets.emplace_back(ip, port);
    } else {
      self_address.port = port;
    }
    ++total_node_num;
  }
  if (total_node_num - 1 < failure_domain_size) {
    exit(-1);
  }
  if ((total_node_num - 1) / zone_num < (rs_m + rs_k)) {
    exit(-1);
  }
  return total_node_num;
}

void TCPNode::start() {
    server = new TCPServer(self_index, self_address.port, node_num - 1, rs_k, rs_m, node_size, zone_num);
    std::thread wait_svr_thr([&] { server->wait_for_connection(); });
    client = new TCPClient(self_index, targets, node_size, zone_num, node_num, rs_k, rs_m);
    std::thread cli_thr([&] { client->start_client(); });
    wait_svr_thr.join();
    std::thread svr_thr([&] { server->start_serving(); });
  if (self_index) {
    char dir_base[64], command[128];
    for (uint16_t client_index = 1; client_index <= node_num - 1; ++client_index) {
      if (self_index == client_index) {
        continue;
      }
      sprintf(dir_base, "test/store/zone%u", self_index % zone_num);
      sprintf(command, "mkdir -p %s;rm %s/* -f", dir_base, dir_base);
      system(command);
    }
    while (true) {
      RepairInfo task = server->get_task();
      client->add_task(task);
      if (task.last_task_flag) {
        break;
      }
    }
    cli_thr.join();
    svr_thr.join();
  } else {
    std::vector<std::vector<RepairInfo>> task_vec; 
    TaskGenerator tg(rs_k, rs_m, zone_num, node_num - 1, node_lifetime, failure_domain_size, sim_duration, fd_vec, zone_vec);
    task_vec = tg.gen_task();
    int task_number = 0;
    for (auto i = 0; i < task_vec.size(); i++) {
      auto &task_package = task_vec[i];
      if (task_package.size() == 0) {
        exit(-1);
      }
      auto size = task_package.size();
      for (auto i = 0; i < size; i++) {
        task_package[i].task_num_of_cur_round = size;
        task_package[i].task_index = i + 1;
        client->add_task(task_package[i]);
        server->add_task(task_package[i]);
        task_number++;
      }
    }
    cli_thr.join();
    svr_thr.join();
  }
}

TCPNode::~TCPNode() {
  delete server;
  delete client;
}
