#pragma once

#include <string>
#include <vector>

struct __attribute__((packed)) session_row_binary_t {
  double time;
  uint64_t location_id;
  double duration;
};
struct session_row_t {
  double time;
  double duration;
  std::string path;
  int line;
  std::string function;
  std::string name;
};

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data);
