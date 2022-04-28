#ifndef TCP_NODE_HH
#define TCP_NODE_HH

#include <string>
#include <vector>

#include "tcp_client.hh"
#include "tcp_server.hh"

class TCPNode {
 private:
  TCPClient *client;
  TCPServer *server;
  uint16_t self_index;
  socket_address self_address;
  std::vector<socket_address> targets;
  uint16_t node_num;

  uint rs_k;
  uint rs_m;
  uint N;

  uint failure_domain_size;
  uint zone_num;
  unsigned long node_size;
  double node_lifetime;
  uint sim_duration;
  static std::vector<std::vector<uint>> fd_vec;
  static std::vector<std::vector<uint>> zone_vec;

 public:
  explicit TCPNode(uint16_t i);
  ~TCPNode();

  uint16_t parse_config(const std::string &fn);
  void start();
};

#endif
