#include "profiler.hpp"

#include <inttypes.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

#include "defines.hpp"

static inline double getDeltaSecs(const auto &delta_t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(delta_t)
             .count() /
         1e9;
}

MeasureScope::~MeasureScope() {
  ProfilingSession::getGlobalInstace().addMeasure(loc, start, std::chrono::steady_clock::now());
}

void ProfilingSession::addMeasure(const LocationID &loc, const time_point &start,
                                  const time_point &end) {
  if (!enabled()) {
    return;
  }
  if (!initialized) [[unlikely]] {
    throw std::runtime_error("Not initialized");
  }

  std::scoped_lock lck(mtx);
  static measure_t serializer;
  serializer.id = loc.locationID;
  serializer.time = getDeltaSecs(start - initializationTime);
  serializer.duration = getDeltaSecs(end - start);
  fwrite(&serializer, sizeof(serializer), 1, session.get());
}
ProfilingSession &ProfilingSession::getGlobalInstace() {
  static ProfilingSession session;
  return session;
}
void ProfilingSession::initialize(const std::string &_outFolder) {
  outFolder = _outFolder;

  session = std::unique_ptr<FILE, FileCloser>(
      fopen((outFolder + "/profiler_session.csv").c_str(), "wb"));
  if (!session) {
    return;
  }
  initialized = true;
  initializationTime = std::chrono::steady_clock::now();
}

ProfilingSession::~ProfilingSession() {
  if (!session) {
    return;
  }

  std::unique_ptr<FILE, FileCloser> outIDMap(
      fopen((outFolder + "/measures_id_map.csv").c_str(), "w"));
  if (!outIDMap) {
    return;
  }
  for (const auto &[location, id] : locationIDMap) {
    fprintf(outIDMap.get(), "%s;%" PRIu64 "\n", location.c_str(), id);
  }
}

void ProfilingSession::enable() { amIEnabled = true; }
void ProfilingSession::disable() { amIEnabled = false; }
bool ProfilingSession::enabled() const { return amIEnabled; }


