#include "profiler.hpp"

#include <algorithm>
#include <cstdint>
#include <inttypes.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

static constexpr size_t kSessionBufferSize = 1 << 20;

static inline constexpr int64_t getDeltaNanos(const auto &delta_t) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(delta_t)
      .count();
}

static thread_local MeasureBuffer tlsMeasureBuffer;

MeasureScope::~MeasureScope() noexcept {
  ProfilingSession::getGlobalInstace().addMeasure(loc, start, std::chrono::steady_clock::now());
}

void ProfilingSession::addMeasure(const LocationID &loc, const time_point &start,
                                  const time_point &end) noexcept {
  if (!enabled()) [[unlikely]] {
    return;
  }
  if (!initialized) [[unlikely]] {
    return;
  }

  const measure_t serializer{
    .time = getDeltaNanos(start - initializationTime),
    .id = loc.locationID,
    .duration = getDeltaNanos(end - start),
  };
  tlsMeasureBuffer.push(serializer);
}

void MeasureBuffer::push(const measure_t &m) noexcept {
  auto &sessionInst = ProfilingSession::getGlobalInstace();
  if (!registered) {
    sessionInst.registerBuffer(this);
    registered = true;
  }
  data[count++] = m;
  if (count == kCapacity) {
    sessionInst.flushBuffer(*this);
  }
}

MeasureBuffer::~MeasureBuffer() noexcept {
  ProfilingSession::getGlobalInstace().retireBuffer(this);
}

void ProfilingSession::registerBuffer(MeasureBuffer *buf) noexcept {
  std::scoped_lock lck(mtx);
  buffers.push_back(buf);
}

void ProfilingSession::retireBuffer(MeasureBuffer *buf) noexcept {
  std::scoped_lock lck(mtx);
  writeLocked(buf->data.data(), buf->count);
  buf->count = 0;
  buffers.erase(std::remove(buffers.begin(), buffers.end(), buf),
               buffers.end());
}

void ProfilingSession::flushBuffer(MeasureBuffer &buf) noexcept {
  std::scoped_lock lck(mtx);
  writeLocked(buf.data.data(), buf.count);
  buf.count = 0;
}

void ProfilingSession::writeLocked(const measure_t *data,
                                   size_t count) noexcept {
  if (!session || count == 0) {
    return;
  }
  fwrite(data, sizeof(measure_t), count, session.get());
}

ProfilingSession &ProfilingSession::getGlobalInstace() noexcept {
  static ProfilingSession session;
  return session;
}
void ProfilingSession::initialize(const std::string &_outFolder) {
  outFolder = _outFolder;

  session = std::unique_ptr<FILE, FileCloser>(
      fopen((outFolder + "/" SESSION_FILENAME).c_str(), "wb"));
  if (!session) {
    return;
  }
  setvbuf(session.get(), nullptr, _IOFBF, kSessionBufferSize);
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
  {
    std::scoped_lock lck(mtx);
    for (MeasureBuffer *buf : buffers) {
      writeLocked(buf->data.data(), buf->count);
      buf->count = 0;
      buf->registered = false;
    }
    buffers.clear();
  }
  std::unique_ptr<FILE, FileCloser> outIDMap(
      fopen((outFolder + "/" SESSION_ID_MAP_FILENAME).c_str(), "w"));
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
