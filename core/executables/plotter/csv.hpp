#pragma once

#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

struct __attribute__((packed)) session_row_binary_t {
  int64_t time;
  uint64_t location_id;
  int64_t duration;
};
struct session_row_t {
  double time;
  double duration;
  std::string_view path;
  int line;
  std::string_view function;
  std::string_view name;
};
struct id_map {
  uint64_t id;
  std::string path;
  int line;
  std::string function;
  std::string name;
};

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data,
                    std::unordered_map<uint64_t, id_map> &locationIDMap,
                    std::atomic<float> &progress);
