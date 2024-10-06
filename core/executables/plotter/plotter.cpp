#include "plotter.hpp"
#include "core/executables/plotter/csv.hpp"
#include "implot.h"
#include "kvp.hpp"
#include <cstdlib>
#include <iostream>

void plotSession(const std::vector<session_row_t> &session);

void Plotter::Draw() {
  if (!sessionCsvValid) {
    std::string &path = KVP::getMutable("base path");
    ImGui::InputText("Base path", &path);
    if (ImGui::Button("Open")) {
      sessionCsvValid = ReadSessionCSV(path, sessionData);
      processSessionData();
    }
  } else {
    plotSession(sessionData);
  }
}

int main(void) {
  Plotter plotter;
  plotter.SetTitle("Profiler plotter");
  plotter.Init();
  plotter.Run();
  plotter.Shutdown();
  return EXIT_SUCCESS;
}

std::string_view extractFunctionName(const std::string &input) {
  std::size_t funcStart = input.find_last_of(')');
  if (funcStart != std::string::npos && funcStart + 2 < input.size()) {
    return std::string_view(input).substr(funcStart + 2);
  }
  return {};
}
std::string_view extractFileAndLine(const std::string &input) {
  std::size_t lineStart = input.find_last_of('(');
  std::size_t lineEnd = input.find_last_of(')');
  std::size_t pathEnd = input.find_last_of('/');

  if (pathEnd != std::string::npos && lineStart != std::string::npos &&
      lineEnd != std::string::npos) {
    return std::string_view(input).substr(pathEnd + 1, lineEnd - pathEnd + 1);
  }
  return {};
}

void Plotter::processSessionData() {
  measurements.clear();
  for (const auto &row : sessionData) {
    measurement_element_t &meas = measurements[row.location];
    meas.timeData.push_back({row.time, row.duration});
    meas.meanDuration += row.duration;
  }
  for (auto &[loc, meas] : measurements) {
    meas.location = loc;
    meas.function = extractFunctionName(loc);
    meas.fileAndLine = extractFileAndLine(loc);
    meas.standardDeviation = 0.0;
    for (const auto &timeData : meas.timeData) {
      meas.standardDeviation +=
          std::pow(timeData.duration - meas.meanDuration, 2.0);
    }
    meas.standardDeviation = std::sqrt(meas.standardDeviation);
    std::cout << meas.fileAndLine << " " << meas.function << ": "
              << meas.meanDuration << " " << meas.standardDeviation
              << std::endl;
  }
}

void plotSession(const std::vector<session_row_t> &session) {
  if (ImPlot::BeginPlot("session")) {

    ImPlot::EndPlot();
  }
}
