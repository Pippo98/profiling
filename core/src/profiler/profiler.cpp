#include "profiler.hpp"

#include <cstdint>
#include <inttypes.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

#include "defines.hpp"

static inline constexpr double getDeltaSecs(const auto &delta_t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(delta_t)
             .count() /
         1e9;
}

MeasureScope::~MeasureScope() noexcept {
  ProfilingSession::getGlobalInstace().addMeasure(loc, start, std::chrono::steady_clock::now());
}

void ProfilingSession::addMeasure(const LocationID &loc, const time_point &start,
                                  const time_point &end) noexcept {
  if (!enabled()) {
    return;
  }
  if (!initialized) [[unlikely]] {
    return;
  }

  const measure_t serializer{
    .time = getDeltaSecs(start - initializationTime),
    .id = loc.locationID,
    .duration = getDeltaSecs(end - start),
  };
  std::scoped_lock lck(mtx);
  fwrite(&serializer, sizeof(serializer), 1, session.get());
}
ProfilingSession &ProfilingSession::getGlobalInstace() noexcept {
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
	close();
}

void ProfilingSession::close() {
  if (!session) {
    return;
  }
	if (!initialized) {
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
	session.reset();
	outIDMap.reset();
	initialized = false;
	amIEnabled = false;
	initializationTime = time_point();
}

void ProfilingSession::enable() { amIEnabled = true; }
void ProfilingSession::disable() { amIEnabled = false; }
bool ProfilingSession::enabled() const { return amIEnabled; }
