#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#if __has_include(<experimental/source_location>)
#include <experimental/source_location>
namespace std {
using namespace experimental;
using namespace experimental::fundamentals_v2;
} // namespace std
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
#define MEASURE_SCOPE(instance_name)                                           \
  static LocationID locId(#instance_name);                                     \
  MeasureScope instance_name(locId);
#else
#define MEASURE_SCOPE(instance_name)
#endif

class LocationID;
class MeasureBuffer;

struct measure_t {
  int64_t time;
  uint64_t id;
  int64_t duration;
};

class ProfilingSession {
private:
  void addMeasure(const LocationID &loc, const time_point &start,
                  const time_point &end) noexcept;

  void addLocation(const char *name, const source_loc &loc,
                   const uint64_t &id) noexcept {
    const std::string sstr = std::string(loc.file_name()) + ";" +
                             std::to_string(loc.line()) + ";" +
                             loc.function_name() + ";" + name;
    locationIDMap[sstr] = id;
  }

  void registerBuffer(MeasureBuffer *buf) noexcept;
  void retireBuffer(MeasureBuffer *buf) noexcept;
  void flushBuffer(MeasureBuffer &buf) noexcept;
  void writeLocked(const measure_t *data, size_t count) noexcept;

  friend class MeasureScope;
  friend class LocationID;
  friend class MeasureBuffer;
  ProfilingSession() = default;

public:
  ~ProfilingSession();

  void initialize(const std::string &outFolder);

  void enable();
  void disable();
  bool enabled() const;
	void close();

  static ProfilingSession &getGlobalInstace() noexcept;

private:
  std::mutex mtx;
  bool amIEnabled = false;
  bool initialized = false;
  std::string outFolder;
  time_point initializationTime;

  std::map<std::string, uint64_t> locationIDMap;
  std::vector<MeasureBuffer *> buffers;

  std::unique_ptr<FILE, FileCloser> session;
};

class MeasureBuffer {
public:
  ~MeasureBuffer() noexcept;
  void push(const measure_t &m) noexcept;

private:
  static constexpr size_t kCapacity = 1024;

  std::array<measure_t, kCapacity> data;
  size_t count = 0;
  bool registered = false;

  friend class ProfilingSession;
};

class LocationID {
public:
  static constexpr unsigned long hash(const source_loc &loc) {
    unsigned long hash = 5381;

    const auto &hashStr = [](unsigned long hash,
                             const char *str) -> unsigned long {
      char c;
      while ((c = *str++)) {
        hash = hash * 33 + c;
      }
      return hash;
    };
    hash = hashStr(hash, loc.file_name());
    hash += loc.line();
    hash = hashStr(hash, loc.function_name());

    return hash;
  }

  LocationID(const char *name,
             const source_loc &loc = std::source_location::current()) noexcept
      : locationID(hash(loc)) {
    ProfilingSession::getGlobalInstace().addLocation(name, loc, locationID);
  }

  const uint64_t locationID;
};

class MeasureScope {
public:
  MeasureScope(const LocationID &_loc) noexcept
      : loc(_loc), start(std::chrono::steady_clock::now()) {}
  ~MeasureScope() noexcept;

private:
  const LocationID &loc;
  const time_point start;
};
