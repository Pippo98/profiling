#pragma once

#include <map>
#include <string>

typedef std::map<std::string, std::string> kvp_t;

class KVP {
private:
  KVP();
  ~KVP();
  KVP(const KVP &other) = delete;
  KVP &operator=(const KVP &other) = delete;
  static KVP &get();

  kvp_t data;

public:
  static bool haveKey(const std::string &key);
  static const std::string &get(const std::string &key);
  static std::string &getMutable(const std::string &key);
  static void set(const std::string &key, const std::string &val);
  static void save();
};
