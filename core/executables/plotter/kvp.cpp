#include "kvp.hpp"

#include "core/executables/plotter/defines.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>

static void KVPSave(const std::string &filename,
                    const std::map<std::string, std::string> &data) {
  std::ofstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Error: Could not open file for writing!" << std::endl;
    return;
  }

  for (const auto &pair : data) {
    file << pair.first << KVP_SEPARATOR << pair.second << std::endl;
  }

  file.close();
}

static std::map<std::string, std::string> KVPLoad(const std::string &filename) {
  kvp_t data;
  std::ifstream file(filename);
  std::string line;

  // Check if the file is open
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file for reading!" << std::endl;
    return data;
  }

  while (std::getline(file, line)) {
    std::size_t separator_pos = line.find(KVP_SEPARATOR);
    if (separator_pos != std::string::npos) {
      std::string key = line.substr(0, separator_pos);
      std::string value = line.substr(separator_pos + 3); // Skip past "@=@"
      data[key] = value;
    }
  }

  file.close();
  return data;
}

KVP::KVP() { data = KVPLoad(KVP_PATH); }
KVP::~KVP() { KVPSave(KVP_PATH, data); }
KVP &KVP::get() {
  static KVP inst;
  return inst;
}

bool KVP::haveKey(const std::string &key) {
  return get().data.find(key) != get().data.end();
}
const std::string &KVP::get(const std::string &key) {
  if (get().haveKey(key)) {
    return get().data.at(key);
  }
  static std::string empty;
  return empty;
}
std::string &KVP::getMutable(const std::string &key) { return get().data[key]; }
void KVP::set(const std::string &key, const std::string &val) {
  get().data[key] = val;
}
