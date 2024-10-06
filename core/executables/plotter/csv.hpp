#pragma once

#include <string>
#include <vector>
struct session_row_t {
  double time;
  std::string location;
  double duration;
};

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data);
