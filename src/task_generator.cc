#include "task_generator.hh"
#include <cstdlib>
#include <ctime>
#include <assert.h>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <chrono>
#include <string.h>

uint TaskGenerator::_zone_num;

TaskGenerator::TaskGenerator(uint k, uint m, uint zone_num, uint data_node_num, double node_lifetime, const uint fd_size, uint sim_duration, std::vector<std::vector<uint> > &fd_vec, std::vector<std::vector<uint> > &zone_vec)
    : _k(k), 
    _m(m),
    _data_node_num(data_node_num), 
    _node_lifetime(node_lifetime),
    _failure_domain_size(fd_size),
    _sim_duration(sim_duration),
    _fd_vec(fd_vec), 
    _zone_vec(zone_vec) {
      _zone_num = zone_num;
    };

std::vector<std::vector<RepairInfo>> TaskGenerator::gen_task () {
  std::vector<std::vector<RepairInfo>> res;
  uint total_task_num = _sim_duration * 12 * 4;
  uint node_lifetime_week = (uint)(_node_lifetime * 12 * 4);
  std::vector<uint> time_stamp;
  uint seed = std::time(0);
  std::srand(seed);
  for (auto i = 0; i < _data_node_num; i++) {
    time_stamp.push_back(rand() % node_lifetime_week);
  }
  for (auto i = 0; i < _zone_num; i++) {
    _zone_vec.push_back(std::vector<uint>{});
  }
  for (auto i = 1; i <= _data_node_num; i++) {
    uint index = getZoneIndex(i);
    _zone_vec[index].push_back(i);
  }
  uint cur_task_num = 0;
  while (cur_task_num < total_task_num) {
    cur_task_num++;
    std::vector<RepairInfo> task_vec;
    std::vector<uint> fail_node_index;
    if (cur_task_num % (12 * 4) == 0) {
      _correlated_failure_rate = (rand() % 6 + 5) / 1000;
      uint failure_num = static_cast<uint>(_data_node_num * _correlated_failure_rate);
      uint fd_num = _fd_vec.size();
      uint cur_fd_index;
      do {
        cur_fd_index = rand() % fd_num;
      } while (_fd_vec[cur_fd_index].size() < failure_num);
      std::shuffle(_fd_vec[cur_fd_index].begin(), _fd_vec[cur_fd_index].end(), std::default_random_engine(seed));
      for (auto i = 0; i < failure_num; i++) {
        fail_node_index.push_back(_fd_vec[cur_fd_index][i]);
      }
    } else {
      for (auto i = 0; i < time_stamp.size(); i++) {
        if (++time_stamp[i] % node_lifetime_week == 0) {
          fail_node_index.push_back(i + 1);
        }
      }
    }
    if (fail_node_index.empty()) {
      continue;
    }
    std::unordered_map<uint, std::vector<uint> > failure_mp;
    for (auto index : fail_node_index) {
      auto zone_index = getZoneIndex(index);
      failure_mp[zone_index].push_back(index);
    }
    for (auto it = failure_mp.begin(); it != failure_mp.end(); it++) {
      RepairInfo task;
      task.target = it->second;
      task.source = getNodeInSameZone(it->second);
      task.source_size = task.source.size();
      task.target_size = task.target.size();
      task.zone_index = it->first;
      task.k = _k;
      task.m = _m;
      task.zone_num = _zone_num;
      task.data_node_num = _data_node_num;
      strcpy(task.read_fn, "test/stdfile");
      sprintf(task.write_fn, "test/store/zone%u/f_%u", getZoneIndex(it->first), cur_task_num);
      task_vec.push_back(task);
    }
    res.push_back(task_vec);
  }
  std::vector<RepairInfo> last_task_vec;
  RepairInfo last_task;
  last_task.last_task_flag = true;
  for (uint i = 1; i <= _data_node_num; i++) {
    last_task.target.emplace_back(i);
  }
  last_task_vec.push_back(last_task);
  res.push_back(last_task_vec);
  return res;
}

std::vector<uint> TaskGenerator::getNodeInSameZone(std::vector<uint> index_vec) {
  std::vector<uint> res;
  if (index_vec.size() > 1) {
    for (auto i = 1; i < index_vec.size(); i++) {
      assert(getZoneIndex(index_vec[i]) == getZoneIndex(index_vec[i - 1]));
    }
  }
  uint zone_index = getZoneIndex(index_vec[0]);
  uint cur_index = zone_index ? zone_index : _zone_num;
  while (cur_index <= _data_node_num) {
    if (find(index_vec.begin(), index_vec.end(), cur_index) == index_vec.end()) {
      res.push_back(cur_index);
    }
    cur_index += _zone_num;
  }
  return res;
}