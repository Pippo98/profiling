#include "plotter.hpp"
#include "defines.hpp"
#include "imgui.h"
#include "implot.h"
#include "kvp.hpp"
#include "utils/style.hpp"
extern "C" {
#include "tinyfiledialogs.h"
}
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

ImFont *h1;
ImFont *h2;
ImFont *h3;
ImFont *text;

void loadImGuiFont() {
  h1 = ImGui::GetIO().Fonts->AddFontFromFileTTF(
      PROJECT_PATH "/assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 16);
  h2 = ImGui::GetIO().Fonts->AddFontFromFileTTF(
      PROJECT_PATH "/assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 18);
  h3 = ImGui::GetIO().Fonts->AddFontFromFileTTF(
      PROJECT_PATH "/assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 22);
  text = ImGui::GetIO().Fonts->AddFontFromFileTTF(
      PROJECT_PATH "/assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 14);
  if (h1 == nullptr) {
    h1 = ImGui::GetFont();
    h2 = ImGui::GetFont();
    h3 = ImGui::GetFont();
    text = ImGui::GetFont();
  }
}

int main(int argc, char **argv) {
  Plotter plotter;
  plotter.SetTitle("Profiler plotter");
  plotter.Init();

  loadImGuiFont();
  Dracula();

  plotter.Run();
  plotter.Shutdown();
  return EXIT_SUCCESS;
}

Plotter::~Plotter() {
  if (loadingThread && loadingThread->joinable()) {
    loadingThread->join();
  }
}

void Plotter::startLoading() {
  shouldStartLoading = false;
  if (loading) {
    return;
  }

  sessionCsvValid = false;

  if (loadingThread && loadingThread->joinable()) {
    loadingThread->join();
  }
  loadingThread = std::make_unique<std::thread>([&]() {
    sessionCsvValid = ReadSessionCSV(loadedPath, sessionData, progress);
    processSessionData();
    loading = false;
  });
  loading = true;
}

void Plotter::Draw() {
  if (shouldStartLoading) {
    startLoading();
  }

  if (loading) {
		ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f - 200,
			                               ImGui::GetIO().DisplaySize.y / 2.0f - 50),
			                        ImGuiCond_Always);
		}

    ImGui::Begin("Loading");
    ImGui::Text("Loading session data from %s", loadedPath.c_str());
    if (!sessionCsvValid) {
      ImGui::Text("Loading CSV file...");
    } else {
      ImGui::Text("Processing data...");
    }
    ImGui::ProgressBar(progress.load(), ImVec2(0.0f, 0.0f));
    ImGui::End();
    return;
  }

  if (!sessionCsvValid) {
		ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Once);
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f - 300,
			                               ImGui::GetIO().DisplaySize.y / 2.0f - 200),
			                        ImGuiCond_Always);
		}
    ImGui::Begin("Open");
    std::string &path = KVP::getMutable("base path");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Path to session data:");
    ImGui::SameLine();
    ImGui::InputText("##base_path", &path);
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
      const char *file =
          tinyfd_selectFolderDialog("Select base path", path.c_str());
      if (file) {
        path = file;
        path += "/";
      }
    }
    if (ImGui::Button("Open") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
      loadedPath = path;
      shouldStartLoading = true;
    }
    ImGui::End();
  } else {
		if(!ImGui::GetCurrentContext()->SettingsLoaded) {
			ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);
		}
    if (ImGui::Begin("Timeline")) {
      plotTimeEvolution();
    }
    ImGui::End();

		if(!ImGui::GetCurrentContext()->SettingsLoaded) {
			ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
		}
    if (ImGui::Begin("Statistics")) {
      plotBars();
    }
    ImGui::End();
  }

  if (exportModalOpen) {
    ImGui::OpenPopup("Export options");
    exportModalOpen = false;
  }
  drawExportModal();
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
  keysByDuration.clear();
  keysByAppearance.clear();
  measurementsPerSecond.resize(sessionData.size());
  std::vector<double> measurementsTimes(sessionData.size());
  for (size_t i = 0; i < sessionData.size(); i++) {
    const auto &row = sessionData[i];

    measurement_element_t &meas = measurements[getLocation(row)];

    meas.function = row.function;
    meas.line = row.line;
    meas.path = row.path;
    meas.file = std::filesystem::path(row.path).filename();
    meas.name = row.name;
    meas.timeData.push_back({row.time, row.duration});
    if (meas.startAndDuration.time == -1) {
      meas.startAndDuration.time = row.time;
    }
    meas.meanDuration += row.duration;
    meas.startAndDuration.duration = row.time + row.duration;

    measurementsTimes[i] = row.time;

    progress = (double)i / sessionData.size();
  }

  if (measurementsTimes.empty()) {
    return;
  }

  std::sort(measurementsTimes.begin(), measurementsTimes.end());
  for (size_t i = 1; i < measurementsTimes.size() - 1; i++) {
    measurementsPerSecond[i].time = measurementsTimes[i];
    measurementsPerSecond[i].value = measurementsPerSecond[i - 1].value +
                                     measurementsTimes[i] -
                                     measurementsTimes[i - 1];
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
    keysByDuration.push_back(loc);
  }
  keysByAppearance = keysByDuration;
  std::sort(keysByDuration.begin(), keysByDuration.end(),
            [&](const auto &a, const auto &b) {
              const auto &elA = measurements[a];
              const auto &elB = measurements[b];
              return elA.meanDuration * elA.timeData.size() >
                     elB.meanDuration * elB.timeData.size();
            });
  std::sort(keysByAppearance.begin(), keysByAppearance.end(),
            [&](const std::string &a, const std::string &b) {
              const measurement_element_t &elA = measurements[a];
              const measurement_element_t &elB = measurements[b];
              return elA.startAndDuration.time < elB.startAndDuration.time;
            });
  for (size_t i = 0; i < keysByDuration.size(); i++) {
    measurements[keysByDuration[i]].durationSortedIndex = i;
  }
  for (size_t i = 0; i < keysByAppearance.size(); i++) {
    measurements[keysByAppearance[i]].appearanceSortedIndex = i;
  }
}

void drawElementTooltip(const measurement_element_t &element,
                        ssize_t timeInstanceId = -1, ImU32 borderColor = 0) {
  if (borderColor == 0) {
    borderColor =
        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
  }
  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("Press Enter to open file preview");
    ImGui::Separator();
    ImGui::Text("Name:");
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(borderColor), "%s",
                       element.name.c_str());
    ImGui::Text("Function:");
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(borderColor), "%s",
                       element.function.c_str());
    ImGui::Text("File and line:");
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(borderColor), "%s:%ld",
                       element.file.c_str(), element.line);
    ImGui::Separator();
    ImGui::Text("Hits: %ld", element.timeData.size());
    ImGui::Text("Mean duration: %0.9f s", element.meanDuration);
    ImGui::Text("Mean frequency: %0.3f Hz", element.meanFrequency);
    ImGui::Text("Cumulative time: %0.9f s",
                element.meanDuration * element.timeData.size());
    if (timeInstanceId != -1) {
      ImGui::Separator();
      ImGui::Text("Hit #: %ld", timeInstanceId);
      ImGui::Text("Time: %0.9f s", element.timeData[timeInstanceId].time);
      ImGui::Text("Duration: %0.9f s",
                  element.timeData[timeInstanceId].duration);
    }
    ImGui::EndTooltip();
  }
}

void Plotter::plotTimeEvolution() {
  static double lowerThreshold = 0.0;
  ImGui::SetNextItemWidth(200);
  drawSortSelector();
  ImGui::SameLine();
  ImGui::Text("Skip samples with duration less than: ");
  ImGui::SameLine();
  ImGui::InputDouble("##skip_samples_every", &lowerThreshold);

  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0f;
  static ImPlotRect limits(0, endTime, 0, 0);
  const size_t maxAllowedSamples{5000};
  static std::map<std::string, size_t> lastFrameSamples;

  ssize_t showTooltip = -1;
  std::string tooltipElement;
  ImU32 tooltipColor = 0;

  float row_ratios[2] = {1.0F / 10, 9.0F / 10};

  if (ImPlot::BeginSubplots("time series", 2, 1, size,
                            ImPlotSubplotFlags_LinkAllX |
                                ImPlotSubplotFlags_NoTitle,
                            row_ratios)) {
    if (ImPlot::BeginPlot("##Measures per second")) {
      ImPlot::SetupAxis(ImAxis_X1, "##time", ImPlotAxisFlags_NoDecorations);
      ImPlot::SetupAxis(ImAxis_Y1, "##Measures per Second",
                        ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_AutoFit);
      double range = limits.X.Size();

      size_t N = std::min(maxAllowedSamples, measurementsPerSecond.size());
      auto start = std::lower_bound(
          measurementsPerSecond.begin(), measurementsPerSecond.end(),
          limits.X.Min, [](const time_value_pair_t<double> &el, double value) {
            return el.time < value;
          });
      auto end = std::lower_bound(measurementsPerSecond.begin(),
                                  measurementsPerSecond.end(), limits.X.Max,
                                  [](const time_value_pair_t<double> &el,
                                     double value) { return el.time < value; });

      auto dist = std::distance(start, end);
      size_t increment = dist / (2 * maxAllowedSamples);
      if (increment == 0) {
        increment++;
      }
      std::vector<time_value_pair_t<double>> means;
      for (auto itr = start; itr < end; itr += increment) {
        means.push_back(
            {.time = itr->time,
             .value = 1.0 / (((itr + 1)->value - itr->value) / 2.0)});
      }

      ImPlot::PlotScatter("Measures per second", &means[0].time,
                          &means[0].value, means.size(), 0, 0,
                          sizeof(time_value_pair_t<double>));
      ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("TimeEvolution")) {
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
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ||
            ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
          perc /= 2.0;
        }
        const auto shiftLimits = [](float perc,
                                    const ImPlotRect &lim) -> ImVec2 {
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

        int sortedRow = row;
        if (sortBy == (int)SortBy::Duration) {
          sortedRow = meas.durationSortedIndex;
        } else if (sortBy == (int)SortBy::Appearance) {
          sortedRow = meas.appearanceSortedIndex;
        }

        auto col = ImPlot::NextColormapColorU32();

        if (limits.Min().y > yIncrement * (sortedRow + 1)) {
          continue;
        }
        if (limits.Max().y < yIncrement * sortedRow) {
          continue;
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

        size_t startIdx = 0;
        size_t i = 0;
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
          ImVec2 rmin = ImPlot::PlotToPixels(
              ImPlotPoint(td.time, yIncrement * sortedRow));
          ImVec2 rmax = ImPlot::PlotToPixels(
              ImPlotPoint(td.time + td.duration, yIncrement * (sortedRow + 1)));
          ImPlotRect rect(rmin.x, rmax.x, rmin.y, rmax.y);
          ImPlot::GetPlotDrawList()->AddRectFilled(rmin, rmax, col);
          const auto &mousePos = ImPlot::GetPlotMousePos();
          if (mousePos.x > td.time && mousePos.x < td.time + td.duration &&
              mousePos.y > yIncrement * sortedRow &&
              mousePos.y < yIncrement * (sortedRow + 1)) {
            showTooltip = i;
            tooltipElement = loc;
            tooltipColor = col;

            if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
              previewFileName = meas.path;
              previewFileLine = meas.line;
            }
          }
        }
        lastFrameSamples[loc] = i - startIdx;
      }

      ImPlot::PopPlotClipRect();

      {
        double xDuration[2] = {0.0, endTime};
        double yLocation[2] = {0.0, 0.0};
        ImPlot::PlotLine("##Duration", xDuration, yLocation, 2);
      }

      row = 0;
      for (auto &[loc, meas] : measurements) {
        int sortedRow = row;
        if (sortBy == (int)SortBy::Duration) {
          sortedRow = meas.durationSortedIndex;
        } else if (sortBy == (int)SortBy::Appearance) {
          sortedRow = meas.appearanceSortedIndex;
        }
        std::string text = meas.name + "\n" + meas.file + ":" +
                           std::to_string(meas.line) + "\n" + meas.function;
        float sizeX = ImGui::CalcTextSize(text.c_str()).x;
        ImPlot::PlotText(text.c_str(), limits.Min().x,
                         (sortedRow + 0.5) * (yIncrement),
                         ImVec2(sizeX / 2.0f, 0));
        row++;
      }

      ImPlot::EndPlot();
    }
    ImPlot::EndSubplots();
  }

  if (showTooltip != -1) {
    drawElementTooltip(measurements[tooltipElement], showTooltip, tooltipColor);
  }

  static bool modalOpened = false;
  static bool modalOpenedNow = false;

  if (!previewFileName.empty() && previewFileLines.empty()) {
    std::ifstream f(previewFileName, std::fstream::in);
    previewFileLines.clear();
    if (f.is_open()) {
      std::string line;
      while (std::getline(f, line)) {
        previewFileLines.push_back(line);
      }
      f.close();
    }
    previewFileName.clear();
    ImGui::OpenPopup("File preview");
    modalOpened = true;
    modalOpenedNow = true;
  }

  if (ImGui::BeginPopupModal("File preview", &modalOpened)) {
    for (int i = 0; i < previewFileLines.size(); i++) {

      ImGui::Text("%04d | ", i + 1);
      ImGui::SameLine();
      if (i + 1 == previewFileLine) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 0.0, 1.0));
      }

      ImGui::TextUnformatted(previewFileLines[i].c_str());

      if (i + 1 == previewFileLine) {
        ImGui::PopStyleColor();
        if (modalOpenedNow) {
          ImGui::SetScrollHereY();
        }
      }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      ImGui::CloseCurrentPopup();
    }

    modalOpenedNow = false;
    ImGui::EndPopup();
  }

  if (!modalOpened) {
    previewFileLines.clear();
  }
}

void Plotter::plotBars() {
  static int opts = 0;

  ImGui::Text("Loaded path: %s", loadedPath.c_str());

  if (ImGui::Button("Close session")) {
    sessionCsvValid = false;
  }
	ImGui::SameLine();
  if (ImGui::Button("Reload")) {
    shouldStartLoading = true;
  }
	ImGui::SameLine();
  if (ImGui::Button("Export")) {
    exportModalOpen = true;
  }
	ImGui::Separator();

  drawSortSelector();
  ImGui::SameLine();
  ImGui::Text("Plot options:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200);
  const char *plotOptions[] = {"Mean", "Cumulative", "Percentage of total time",
                               "Counts", "Frequency"};
  ImGui::Combo("##Plot options", &opts, plotOptions, IM_ARRAYSIZE(plotOptions));

  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0F;
  if (ImPlot::BeginPlot("session", size)) {
    std::vector<double> yPosition(measurements.size());
    std::vector<double> bar(measurements.size());
    std::vector<double> std(measurements.size());

    int row = 0;
    for (auto &[loc, meas] : measurements) {
      yPosition[row] = row;
      if (sortBy == (int)SortBy::Duration) {
        yPosition[row] = meas.durationSortedIndex;
      } else if (sortBy == (int)SortBy::Appearance) {
        yPosition[row] = meas.appearanceSortedIndex;
      }
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

    const char *name = plotOptions[opts];
    ImPlot::PlotBars(name, bar.data(), yPosition.data(), bar.size(),
                     yIncrement / 2.0, ImPlotBarsFlags_Horizontal);
    if (opts == 0) {
      ImPlot::PlotErrorBars("Standard deviation", bar.data(), yPosition.data(),
                            std.data(), bar.size(),
                            ImPlotErrorBarsFlags_Horizontal);
    }

    row = 0;
    auto pltMin = ImPlot::GetPlotLimits();
    for (auto &[loc, meas] : measurements) {
      int sortedRow = row;
      if (sortBy == (int)SortBy::Duration) {
        sortedRow = meas.durationSortedIndex;
      } else if (sortBy == (int)SortBy::Appearance) {
        sortedRow = meas.appearanceSortedIndex;
      }
      std::string text = meas.name + "\n" + meas.file + ":" +
                         std::to_string(meas.line) + "\n" + meas.function;
      float sizeX = ImGui::CalcTextSize(text.c_str()).x;
      ImPlot::PlotText(text.c_str(), pltMin.Min().x, sortedRow * (yIncrement),
                       ImVec2(sizeX / 2.0F, 0));
      row++;
    }

    ImPlot::EndPlot();
  }
}

void Plotter::drawSortSelector() {
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Sort by:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200);
  ImGui::Combo("##Sort by", &sortBy, "No sort\0By duration\0By appearance\0");
}

void Plotter::drawExportModal() {
  static std::string prefix = "export";
  if (ImGui::BeginPopupModal("Export options", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Export prefix: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##export_file_name", &prefix);
    std::string exportFileName = prefix + "_session.csv";
    std::string exportStatsFileName = prefix + "_stats.csv";
    ImGui::Text("Files will be called: %s and %s and will be saved in %s",
                exportFileName.c_str(), exportStatsFileName.c_str(),
                loadedPath.c_str());

    static bool exportStats = true;
    static bool exportSession = true;
    ImGui::Checkbox("Export session data", &exportSession);
    ImGui::SameLine();
    ImGui::Checkbox("Export stats", &exportStats);

    if (ImGui::Button("Export")) {
      if (exportSession && !exportFileName.empty()) {
        std::ofstream out(loadedPath + "/" + exportFileName, std::ios::out);
        if (out.is_open()) {
          out << "time;duration;path;line;function;name\n";
          for (const auto &row : sessionData) {
            out << row.time << ";" << row.duration << ";" << row.path << ";"
                << row.line << ";" << row.function << ";" << row.name << "\n";
          }
          out.close();
        } else {
          std::cerr << "Error: Could not open file for writing!" << std::endl;
        }
      } else {
        if (exportFileName.empty()) {
          std::cerr << "Error: Export file name cannot be empty!" << std::endl;
        }
      }

      if (exportStats && !exportStatsFileName.empty()) {
        std::ofstream out(loadedPath + "/" + exportStatsFileName,
                          std::ios::out);
        if (out.is_open()) {
          out << "name;function;file;line;mean duration;standard deviation;"
                 "mean frequency;hits\n";
          for (const auto &[loc, meas] : measurements) {
            out << meas.name << ";" << meas.function << ";" << meas.file << ";"
                << meas.line << ";" << meas.meanDuration << ";"
                << meas.standardDeviation << ";" << meas.meanFrequency << ";"
                << meas.timeData.size() << "\n";
          }
          out.close();
        } else {
          if (exportStatsFileName.empty()) {
            std::cerr << "Error: Could not open stats file for writing!"
                      << std::endl;
          }
        }
      } else {
        std::cerr << "Error: Export stats file name cannot be empty!"
                  << std::endl;
      }

      ImGui::CloseCurrentPopup();
    }

    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
