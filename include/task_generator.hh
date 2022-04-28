#pragma once
#include <iostream>
#include <vector>
#include "repair_info.hh"

class TaskGenerator {
 private:
  uint _k;
  uint _m;
  uint _data_node_num;
  double _node_lifetime;
  uint _correlated_failure_rate;
  uint _sim_duration;
  static uint _zone_num;
  const uint _failure_domain_size;
  std::vector<std::vector<uint> > &_fd_vec;
  std::vector<std::vector<uint> > & _zone_vec;
 public:
  TaskGenerator() = default;
  TaskGenerator(uint k, uint m, uint zone_num, uint data_node_num, double node_lifetime, const uint fd_size, uint sim_duration, std::vector<std::vector<uint> > &fd_vec, std::vector<std::vector<uint>> &zone_vec);
  ~TaskGenerator() = default;
  std::vector<std::vector<RepairInfo>> gen_task();
  static inline uint getZoneIndex(uint node_index){
    return node_index % _zone_num;
  }
  std::vector<uint> getNodeInSameZone(std::vector<uint> index_vec);
};

