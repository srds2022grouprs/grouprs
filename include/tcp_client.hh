#ifndef TCP_CLIENT_HH
#define TCP_CLIENT_HH

#include <sys/time.h>

#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "sockpp/tcp_connector.h"
#include "thread_pool.hh"
#include "repair_info.hh"
#include "task_generator.hh"

typedef struct _s_a {
  std::string ip;
  in_port_t port;
  _s_a(const std::string& _ip, in_port_t _p) : ip(_ip), port(_p){};
  _s_a() = default;
} socket_address;

class TCPClient : public ThreadPool<RepairInfo> {
 private:
  uint16_t node_index;
  unsigned long node_size_byte;
  in_port_t dest_port;
  std::string dest_host;
  std::unique_ptr<sockpp::tcp_connector[]> node_conns;
  std::unique_ptr<std::mutex[]> req_mtxs;
  uint16_t conn_num;
  uint zone_num;
  uint node_num, rs_k, rs_m;

  void connect_one(sockpp::inet_address sock_addr, uint16_t index);

 public:
  TCPClient();
  TCPClient(uint16_t i, std::vector<socket_address>& sa, unsigned long node_size, uint zone_num, uint node_num, uint rs_k, uint rs_m, uint thr_num = 1);
  ~TCPClient();

  void run();
  void start_client();
};

#endif