#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <string.h>

class RepairInfo {
 public:
  bool isHelper;
  std::vector<uint> source;
  std::vector<uint> target;
  uint source_size, target_size;
  uint zone_index;
  bool finish_flag = false;
  bool last_task_flag = false;
  uint task_num_of_cur_round;
  uint task_index;

  uint k;
  uint m;
  uint zone_num;
  uint data_node_num;

  char* mem_ptr = nullptr;
  char read_fn[32] = {0};
  char write_fn[32] = {0};

  RepairInfo() : mem_ptr(nullptr){};

  RepairInfo(const RepairInfo & ri) {
    this->isHelper = ri.isHelper;
    for (auto i : ri.source) {
      this->source.push_back(i);
    }
    for (auto i : ri.target) {
      this->target.push_back(i);
    }
    this->source_size = ri.source_size;
    this->target_size = ri.target_size;
    this->zone_index = ri.zone_index;
    this->finish_flag = ri.finish_flag;
    this->last_task_flag = ri.last_task_flag;
    this->k = ri.k;
    this->m = ri.m;
    this->zone_num = ri.zone_num;
    this->data_node_num = ri.data_node_num;
    this->mem_ptr = ri.mem_ptr;
    this->task_index = ri.task_index;
    this->task_num_of_cur_round = ri.task_num_of_cur_round;
    strcpy(this->read_fn, ri.read_fn);
    strcpy(this->write_fn, ri.write_fn);
  }

  RepairInfo & operator=(const RepairInfo & ri){
    if (this == &ri) {
      return *this;
    }
    this->isHelper = ri.isHelper;
    this->source_size = ri.source_size;
    this->target_size = ri.target_size;
    this->zone_index = ri.zone_index;
    this->finish_flag = ri.finish_flag;
    this->last_task_flag = ri.last_task_flag;
    this->k = ri.k;
    this->m = ri.m;
    this->zone_num = ri.zone_num;
    this->data_node_num = ri.data_node_num;
    this->mem_ptr = ri.mem_ptr;
    this->task_index = ri.task_index;
    this->task_num_of_cur_round = ri.task_num_of_cur_round;
    strcpy(this->read_fn, ri.read_fn);
    strcpy(this->write_fn, ri.write_fn);
    return *this;
  }
};