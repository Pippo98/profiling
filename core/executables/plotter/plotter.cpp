#include "plotter.hpp"
#include "core/executables/plotter/csv.hpp"
#include "imgui.h"
#include "implot.h"
#include "kvp.hpp"
#include <cstdlib>
#include <iostream>

void Plotter::Draw() {
  ImGui::Begin("Plotter");
  if (!sessionCsvValid) {
    std::string &path = KVP::getMutable("base path");
    ImGui::InputText("Base path", &path);
    if (ImGui::Button("Open")) {
      sessionCsvValid = ReadSessionCSV(path, sessionData);
      processSessionData();
    }
  } else {
    plotSession();
  }
  ImGui::End();
}

int main(void) {
  Plotter plotter;
  plotter.SetTitle("Profiler plotter");
  plotter.Init();
  plotter.Run();
  plotter.Shutdown();
  return EXIT_SUCCESS;
}

std::string extractFunctionName(const std::string &input) {
  std::size_t funcStart = input.find_last_of(')');
  if (funcStart != std::string::npos && funcStart + 2 < input.size()) {
    return input.substr(funcStart + 2);
  }
  return {};
}
std::string extractFileAndLine(const std::string &input) {
  std::size_t lineStart = input.find_last_of('(');
  std::size_t lineEnd = input.find_last_of(')');
  std::size_t pathEnd = input.find_last_of('/');

  if (pathEnd != std::string::npos && lineStart != std::string::npos &&
      lineEnd != std::string::npos) {
    return input.substr(pathEnd + 1, lineEnd - pathEnd + 1);
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
  endTime = 0.0;
  for (auto &[loc, meas] : measurements) {
    meas.location = loc;
    meas.function = extractFunctionName(loc);
    meas.fileAndLine = extractFileAndLine(loc);
    meas.standardDeviation = 0.0;
    endTime = std::max(endTime, meas.timeData.back().time);
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

void Plotter::plotSession() {
  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0f;
  if (ImPlot::BeginPlot("session", size)) {
    ImPlot::GetCurrentContext()->CurrentItems->ColormapIdx = 0;
    ImPlot::PushPlotClipRect();
    int row = 0;
    for (auto &[loc, meas] : measurements) {
      auto col = ImPlot::NextColormapColorU32();
      for (int i = 0; i < meas.timeData.size(); i++) {
        const auto &td = meas.timeData[i];
        ImVec2 rmin =
            ImPlot::PlotToPixels(ImPlotPoint(td.time, yIncrement * row));
        ImVec2 rmax = ImPlot::PlotToPixels(
            ImPlotPoint(td.time + td.duration, yIncrement * (row + 1)));
        ImPlot::GetPlotDrawList()->AddRectFilled(rmin, rmax, col);
      }
      row++;
    }

    ImPlot::PopPlotClipRect();

    {
      double xDuration[2] = {0.0, endTime};
      double yLocation[2] = {0.0, 0.0};
      ImPlot::PlotLine("Duration", xDuration, yLocation, 2);
    }

    row = 0;
    for (auto &[loc, meas] : measurements) {
      auto pltMin = ImPlot::GetPlotLimits();
      std::string text = meas.fileAndLine + meas.function;
      ImPlot::PlotText(text.c_str(), pltMin.Min().x, (row + 0.5) * (yIncrement),
                       ImVec2(100, 0));
      row++;
    }

    ImPlot::EndPlot();
  }
}
