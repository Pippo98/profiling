#include "plotter.hpp"
#include "core/executables/plotter/csv.hpp"
#include "imgui.h"
#include "implot.h"
#include "kvp.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>

void Plotter::Draw() {
  if (!sessionCsvValid) {
    ImGui::Begin("Open");
    std::string &path = KVP::getMutable("base path");
    ImGui::InputText("Base path", &path);
    if (ImGui::Button("Open") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
      sessionCsvValid = ReadSessionCSV(path, sessionData);
      loadedPath = path;
      processSessionData();
    }
    ImGui::End();
  } else {
    if (ImGui::Begin("Session")) {
      plotTimeEvolution();
    }
    ImGui::End();
    if (ImGui::Begin("Bars")) {
      plotBars();
    }
    ImGui::End();
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
    measurement_element_t &meas = measurements[getLocation(row)];
    meas.function = row.function;
    meas.line = row.line;
    meas.path = row.path;
    meas.file = std::filesystem::path(row.path).filename();
    meas.timeData.push_back({row.time, row.duration});
    if(meas.startAndDuration.time == -1){
        meas.startAndDuration.time = row.time;
    }
    meas.meanDuration += row.duration;
    meas.startAndDuration.duration = row.time + row.duration;
  }
  endTime = 0.0;
  for (auto &[loc, meas] : measurements) {
    meas.standardDeviation = 0.0;
    meas.meanFrequency = meas.timeData.size() / meas.startAndDuration.duration;
    meas.meanDuration /= meas.timeData.size();
    endTime = std::max(endTime, meas.timeData.back().time);
    for (const auto &timeData : meas.timeData) {
      meas.standardDeviation +=
          std::pow(timeData.duration - meas.meanDuration, 2.0);
    }
    meas.standardDeviation = std::sqrt(meas.standardDeviation);
    std::cout << getLocation(meas) << " => " << meas.meanDuration << " "
              << meas.standardDeviation << std::endl;
  }
}

void drawElementTooltip(const measurement_element_t &element, ssize_t timeInstanceId=-1) {
  if(ImGui::BeginItemTooltip()) {
    
    ImGui::Text("Function: %s", element.function.c_str());
    ImGui::Text("File and line: %s:%ld", element.file.c_str(), element.line);
    ImGui::Separator();
    ImGui::Text("Hits: %ld", element.timeData.size());
    ImGui::Text("Mean duration: %0.9f", element.meanDuration);
    ImGui::Text("Mean frequency: %0.3f", element.meanFrequency);
    ImGui::Text("Cumulative time: %0.9f", element.meanDuration * element.timeData.size());
    if(timeInstanceId != -1) {
      ImGui::Separator();
      ImGui::Text("Hit #: %ld", timeInstanceId);
      ImGui::Text("Time: %0.9f s", element.timeData[timeInstanceId].time);
      ImGui::Text("Duration: %0.9f s", element.timeData[timeInstanceId].duration);
    }
    
    ImGui::EndTooltip();
  }
}

void Plotter::plotTimeEvolution() {

  static double lowerThreshold = 0.0;
  ImGui::SetNextItemWidth(200);
  ImGui::InputDouble("Skip samples with duration less than", &lowerThreshold);

  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0f;
  static ImPlotRect limits(0, endTime, 0, 0);
  const size_t maxAllowedSamples{1000};
  static std::map<std::string, size_t> lastFrameSamples;

  ssize_t showTooltip = -1;
  std::string tooltipElement = "";

  if (ImPlot::BeginPlot("TimeEvolution", size)) {
    ImPlot::SetupAxis(ImAxis_X1, "time [s]", ImPlotAxisFlags_NoGridLines);
    ImPlot::SetupAxis(ImAxis_Y1, "##measurement point",
                      ImPlotAxisFlags_NoGridLines);

    static bool wasHovered;

    if (wasHovered) {
      float perc = 0.008;
      if (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
          ImGui::IsKeyDown(ImGuiKey_RightShift)) {
        perc *= 3.0;
      }
      const auto shiftLimits = [](float perc, const ImPlotRect &lim) -> ImVec2 {
        float range = lim.Max().x - lim.Min().x;
        return ImVec2(lim.Min().x + perc * range, lim.Max().x + perc * range);
      };
      if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        auto newLim = shiftLimits(-perc, limits);
        ImPlot::SetupAxisLimits(ImAxis_X1, newLim.x, newLim.y,
                                ImGuiCond_Always);
      } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        auto newLim = shiftLimits(perc, limits);
        ImPlot::SetupAxisLimits(ImAxis_X1, newLim.x, newLim.y,
                                ImGuiCond_Always);
      }
    }
    wasHovered = ImPlot::IsPlotHovered();
    limits = ImPlot::GetPlotLimits();
    auto limitsPixels = ImPlot::PlotToPixels(limits.Min());

    ImPlot::GetCurrentContext()->CurrentItems->ColormapIdx = 0;
    ImPlot::PushPlotClipRect();
    int row = -1;
    for (auto &[loc, meas] : measurements) {
      row++;
      auto col = ImPlot::NextColormapColorU32();

      if (limits.Min().y > yIncrement * (row + 1)) {
        continue;
      }
      if (limits.Max().y < yIncrement * row) {
        break;
      }

      size_t lastSamples = lastFrameSamples[loc];
      lastFrameSamples[loc] = 0;
      int skipEvery = 0;
      if (lastSamples > maxAllowedSamples) {
        double ratio =
            (double)(maxAllowedSamples) / (lastSamples - maxAllowedSamples);
        if (ratio > 1) {
          skipEvery = 1 + (int)ratio;
        } else {
          skipEvery = -1.0 / ratio;
        }
        if (skipEvery == 0)
          skipEvery++;
      }

      size_t startIdx = 0, i = 0;
      for (i = 0; i < meas.timeData.size();
           skipEvery >= 0 ? i++ : i += -skipEvery) {
        const auto &td = meas.timeData[i];
        if (td.time + td.duration < limits.Min().x) {
          continue;
        }
        if (td.time > limits.Max().x) {
          break;
        }
        if (!startIdx) {
          startIdx = i;
        }
        if (td.duration < lowerThreshold) {
          startIdx++;
          continue;
        }

        if (skipEvery != 0) {
          if (skipEvery > 0 && lastFrameSamples[loc] % skipEvery == 0) {
            continue;
          } else if (skipEvery < 0 &&
                     lastFrameSamples[loc] % (-skipEvery) != 0) {
            continue;
          }
        }
        ImVec2 rmin =
            ImPlot::PlotToPixels(ImPlotPoint(td.time, yIncrement * row));
        ImVec2 rmax = ImPlot::PlotToPixels(
            ImPlotPoint(td.time + td.duration, yIncrement * (row + 1)));
        ImPlotRect rect(rmin.x, rmax.x, rmin.y, rmax.y);
        ImPlot::GetPlotDrawList()->AddRectFilled(rmin, rmax, col);
        const auto &mousePos = ImPlot::GetPlotMousePos();
        if (mousePos.x > td.time && mousePos.x < td.time + td.duration && mousePos.y > yIncrement * row && mousePos.y < yIncrement * (row + 1)) {
          showTooltip = i;
          tooltipElement = loc;
        }
      }
      lastFrameSamples[loc] = i - startIdx;
    }

    ImPlot::PopPlotClipRect();

    {
      double xDuration[2] = {0.0, endTime};
      double yLocation[2] = {0.0, 0.0};
      ImPlot::PlotLine("Duration", xDuration, yLocation, 2);
    }

    row = 0;
    for (auto &[loc, meas] : measurements) {
      std::string text =
          meas.file + " (" + std::to_string(meas.line) + "): " + meas.function;
      float sizeX = ImGui::CalcTextSize(text.c_str()).x;
      ImPlot::PlotText(text.c_str(), limits.Min().x, (row + 0.5) * (yIncrement),
                       ImVec2(sizeX / 2.0f, 0));
      row++;
    }

    ImPlot::EndPlot();
  }

  if(showTooltip != -1) {
    drawElementTooltip(measurements[tooltipElement], showTooltip);
  }
}

void Plotter::plotBars() {
  ImGui::Text("Loaded path: %s", loadedPath.c_str());
  ImGui::SameLine();
  if(ImGui::Button("Close")){
    sessionCsvValid = false;
  }
  static int opts = 0;
  ImGui::RadioButton("Mean", &opts, 0);
  ImGui::RadioButton("Cumulative", &opts, 1);
  ImGui::RadioButton("Percentage", &opts, 2);
  ImGui::RadioButton("Counts", &opts, 3);
  ImGui::RadioButton("Frequency", &opts, 4);
  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0f;
  if (ImPlot::BeginPlot("session", size)) {
    std::vector<double> yPosition(measurements.size());
    std::vector<double> bar(measurements.size());
    std::vector<double> std(measurements.size());

    int row = 0;
    for (auto &[loc, meas] : measurements) {
      yPosition[row] = row;
      if (opts == 0) {
        bar[row] = meas.meanDuration;
        std[row] = meas.standardDeviation;
      } else if (opts == 1) {
        bar[row] = meas.meanDuration * meas.timeData.size();
      } else if (opts == 2) {
        bar[row] = (meas.meanDuration * meas.timeData.size()) / endTime * 100.0;
      } else if (opts == 3) {
        bar[row] = meas.timeData.size();
      } else if (opts == 4) {
        bar[row] = meas.meanFrequency;
      } else {
        bar[row] = 0;
      }
      row++;
    }

    ImPlot::PlotBars(opts == 0 ? "Mean" : "Cumulative", bar.data(),
                     yPosition.data(), bar.size(), yIncrement / 2.0,
                     ImPlotBarsFlags_Horizontal);
    if (opts == 0) {
      ImPlot::PlotErrorBars("Standard deviation", bar.data(), yPosition.data(),
                            std.data(), bar.size(),
                            ImPlotErrorBarsFlags_Horizontal);
    }

    row = 0;
    auto pltMin = ImPlot::GetPlotLimits();
    for (auto &[loc, meas] : measurements) {
      std::string text =
          meas.file + " (" + std::to_string(meas.line) + "): " + meas.function;
      float sizeX = ImGui::CalcTextSize(text.c_str()).x;
      ImPlot::PlotText(text.c_str(), pltMin.Min().x, (row) * (yIncrement),
                       ImVec2(sizeX / 2.0f, 0));
      row++;
    }

    ImPlot::EndPlot();
  }
}
