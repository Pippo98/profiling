#pragma once

#include <map>

#include "app_utils/app.hpp"
#include "core/executables/plotter/csv.hpp"

struct measurement_element_t {
  struct time_and_duration {
    double time;
    double duration;
  };
  std::vector<time_and_duration> timeData;
  double meanDuration;
  double standardDeviation;

  std::string path;
  std::string file;
  uint64_t line;
  std::string function;
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
};
