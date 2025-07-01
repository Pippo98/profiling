#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <memory>

#if __has_include(<experimental/source_location>)
#include <experimental/source_location>
namespace std {
using namespace experimental;
using namespace experimental::fundamentals_v2;
}
#else
#include <source_location>
#endif

typedef std::source_location source_loc;

using time_point = std::chrono::steady_clock::time_point;

struct FileCloser {
  void operator()(FILE *file) const {
    if (file) {
      fclose(file);
    }
  }
};

#if ENABLE_PROFILING == true
#define MEASURE_SCOPE(instance_name) static LocationID locId; MeasureScope instance_name(locId);
#else
#define MEASURE_SCOPE(instance_name)
#endif

class LocationID;

struct measure_t {
  double time;
  uint64_t id;
  double duration;
};

class ProfilingSession {
 private:
  void addMeasure(const LocationID &loc, const time_point &start,
                  const time_point &end);

  uint64_t getLocationID(const source_loc &loc) {
    const std::string sstr = std::string(loc.file_name()) + ";" +
                             std::to_string(loc.line()) + ";" +
                             loc.function_name();
    if (locationIDMap.find(sstr) == locationIDMap.end()) {
      locationIDMap[sstr] = locationIDCount;
      locationIDCount++;
    }
    return locationIDMap.at(sstr);
  }

  friend class MeasureScope;
  friend class LocationID;
  ProfilingSession() = default;

 public:
  ~ProfilingSession();

  void initialize(const std::string &outFolder);

  void enable();
  void disable();
  bool enabled() const;

  static ProfilingSession &getGlobalInstace();
 private:
  std::mutex mtx;
  bool amIEnabled;
  bool initialized;
  std::string outFolder;
  time_point initializationTime;

  uint64_t locationIDCount = 0;
  std::map<std::string, uint64_t> locationIDMap;

  std::unique_ptr<FILE, FileCloser> session;
};

class LocationID {
public:
  LocationID(const source_loc &loc = std::source_location::current()) :
  locationID(ProfilingSession::getGlobalInstace().getLocationID(loc)) {}

  const uint64_t locationID;
};

class MeasureScope {
 public:
  MeasureScope(const LocationID &_loc)
      : loc(_loc), start(std::chrono::steady_clock::now()) {}
  ~MeasureScope();

 private:
  const LocationID &loc;
  const time_point start;
};
