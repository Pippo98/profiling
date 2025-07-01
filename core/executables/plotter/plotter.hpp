#pragma once

#include <map>

#include "imgui.h"
#include "app_utils/app.hpp"
#include "core/executables/plotter/csv.hpp"

extern ImFont *h1;
extern ImFont *h2;
extern ImFont *h3;
extern ImFont *text;

template<typename ValueType>
struct time_value_pair_t{
  double time;
  ValueType value;
};

struct measurement_element_t {
  struct time_and_duration {
    double time = -1;
    double duration = 0.0;
  };
  time_and_duration startAndDuration;
  std::vector<time_and_duration> timeData;
  double meanDuration;
  double standardDeviation;
  double meanFrequency;

  std::string path;
  std::string file;
  uint64_t line;
  std::string function;

  size_t durationSortedIndex;
};
inline std::string getLocation(const measurement_element_t &el) {
  return el.path + "(" + std::to_string(el.line) + "): " + el.function;
}
inline std::string getLocation(const session_row_t &el) {
  return el.path + "(" + std::to_string(el.line) + "): " + el.function;
}

class Plotter : public App {
protected:
  virtual void Draw();

private:
  void processSessionData();
  void plotTimeEvolution();
  void plotBars();

  bool sessionCsvValid = false;
  std::vector<session_row_t> sessionData;
  std::map<std::string, measurement_element_t> measurements;
  double endTime;
  std::string loadedPath;

  // list of measuresPerSeconds along the full log. measures how 
  // many rows per seconds there were.
  // Drops in this values means that nothing happened in those instances
  std::vector<time_value_pair_t<double>> measurementsPerSecond;
  std::vector<time_value_pair_t<double>> measurementsPerSecondAvg;
  std::vector<std::string> keysByDuration;
};
