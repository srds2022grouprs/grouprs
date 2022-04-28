#ifndef TCP_SERVER_HH
#define TCP_SERVER_HH

#include <sys/time.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "memory_pool.hh"
#include "sockpp/tcp_acceptor.h"
#include "thread_pool.hh"
#include "repair_info.hh"

class TCPServer : public ThreadPool<RepairInfo> {
 private:
  uint8_t rs_k;
  uint8_t rs_m;
  uint serving_num;
  in_port_t port;
  unsigned long node_size_byte;
  sockpp::tcp_acceptor acc;
  MemoryPool mem_pool;
  std::mutex serving_mtx;
  std::condition_variable serving_cv;
  bool serving_flag;
  uint blocks_num;
  uint finished_num;
  uint zone_num;
  std::mutex num_mtx;
  std::unique_ptr<std::thread[]> thr; 
  void run(uint16_t client_index);

  uint self_index;
  std::unique_ptr<sockpp::tcp_socket[]> socks;
  uint received_data_num = 0;
  std::mutex receive_data_finish_mtx;
  std::condition_variable receive_data_finish_cv;
  std::vector<char*> data_buffer;
  std::mutex start_to_receive_data_mtx;
  std::condition_variable start_to_receive_data_cv;
  bool start_to_receive_data_flag = false;
  bool serving_finish_flag = false;
  bool serving_finish_flag_2 = false;
  RepairInfo cur_task;
  
  std::vector<uint> requestor_index_vec;
  std::mutex read_requestor_index_mtx;
  std::condition_variable read_requestor_index_cv;
  bool read_requestor_index_flag = false;
  uint received_finish_signal_num = 0;
  std::mutex received_finish_signal_num_mtx;
  std::condition_variable received_finish_signal_num_cv;
  std::mutex cur_task_finish_mtx;
  std::condition_variable cur_task_finish_cv;
  bool cur_task_finish_flag;
  uint finish_thread_num = 0;
  std::mutex finish_thread_mutex;
  std::condition_variable finish_thread_cv;
  uint finish_thread_num_2 = 0;
  std::mutex finish_thread_mutex_2;
  std::condition_variable finish_thread_cv_2;

 public: 
  TCPServer() = default;
  TCPServer(uint node_index, in_port_t _port, uint n, uint8_t k, uint8_t m, unsigned long node_size, uint zone_num);
  ~TCPServer();

  void start_serving();
  void wait_for_connection();
};

#endif