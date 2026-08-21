#include "plotter.hpp"
#include "embedded_font.hpp"
#include "kvp.hpp"
#include "utils/style.hpp"
extern "C" {
#include "tinyfiledialogs.h"
}

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <inttypes.h>

ImFont *h1;
ImFont *h2;
ImFont *h3;
ImFont *text;

void loadImGuiFont() {
  h1 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(
      JetBrainsMonoNerdFontRegular_compressed_data_base85, 16);
  h2 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(
      JetBrainsMonoNerdFontRegular_compressed_data_base85, 18);
  h3 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(
      JetBrainsMonoNerdFontRegular_compressed_data_base85, 22);
  text = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(
      JetBrainsMonoNerdFontRegular_compressed_data_base85, 14);
  if (h1 == nullptr) {
    h1 = ImGui::GetFont();
    h2 = ImGui::GetFont();
    h3 = ImGui::GetFont();
    text = ImGui::GetFont();
  }
}

int main() {
  Plotter plotter;
  plotter.SetTitle("Profiler plotter");
  plotter.Init();

  loadImGuiFont();
  Dracula();

  plotter.Run();
  plotter.Shutdown();
  return EXIT_SUCCESS;
}

void Plotter::startLoading(SessionState &session) {
  session.shouldStartLoading = false;
  if (session.loading) {
    return;
  }

  session.sessionCsvValid = false;

  if (session.loadingThread && session.loadingThread->joinable()) {
    session.loadingThread->join();
  }
  session.loadingThread = std::make_unique<std::thread>([this, &session]() {
    session.sessionCsvValid = ReadSessionCSV(
        session.loadedPath, session.sessionData, session.locationIDMap,
        session.progress);
    processSessionData(session);
    session.loading = false;
  });
  session.loading = true;
}

bool Plotter::drawPathPicker(const char *idLabel, std::string &path) {
  ImGui::PushID(idLabel);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Path to session data:");
  ImGui::SameLine();
  bool enterPressed =
      ImGui::InputText("##path", &path, ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SameLine();
  if (ImGui::Button("Browse")) {
    const char *file =
        tinyfd_selectFolderDialog("Select base path", path.c_str());
    if (file) {
      path = file;
      path += "/";
    }
  }
  ImGui::SameLine();
  bool openClicked = ImGui::Button("Open");
  ImGui::PopID();
  return enterPressed || openClicked;
}

void Plotter::Draw() {
  if (primary.shouldStartLoading) {
    startLoading(primary);
  }

  if (primary.loading) {
    ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
      ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f - 200,
                                     ImGui::GetIO().DisplaySize.y / 2.0f - 50),
                              ImGuiCond_Always);
    }

    ImGui::Begin("Loading");
    ImGui::Text("Loading session data from %s", primary.loadedPath.c_str());
    if (!primary.sessionCsvValid) {
      ImGui::Text("Loading CSV file...");
    } else {
      ImGui::Text("Processing data...");
    }
    ImGui::ProgressBar(primary.progress.load(), ImVec2(0.0f, 0.0f));
    ImGui::End();
    return;
  }

  if (!primary.sessionCsvValid) {
    ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Once);
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
      ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f - 300,
                                     ImGui::GetIO().DisplaySize.y / 2.0f - 200),
                              ImGuiCond_Always);
    }
    ImGui::Begin("Open");
    std::string &path = KVP::getMutable("base path");
    if (drawPathPicker("primary_path_picker", path)) {
      primary.loadedPath = path;
      primary.shouldStartLoading = true;
    }
    ImGui::End();
  } else {
    if (!ImGui::GetCurrentContext()->SettingsLoaded) {
      ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_Once);
    }
    if (ImGui::Begin("Timeline")) {
      plotTimeEvolution();
    }
    ImGui::End();

    if (!ImGui::GetCurrentContext()->SettingsLoaded) {
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
    }
    if (ImGui::Begin("Statistics")) {
      plotBars();
    }
    ImGui::End();

    if (!ImGui::GetCurrentContext()->SettingsLoaded) {
      ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
    }
    if (ImGui::Begin("Compare")) {
      drawCompare();
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

double percentileFromSorted(const std::vector<double> &sortedValues, double p) {
  if (sortedValues.empty()) {
    return 0.0;
  }
  size_t idx = static_cast<size_t>(p / 100.0 * (sortedValues.size() - 1));
  return sortedValues[idx];
}

bool containsCaseInsensitive(const std::string &haystack,
                             const std::string &needle) {
  auto it = std::search(
      haystack.begin(), haystack.end(), needle.begin(), needle.end(),
      [](unsigned char a, unsigned char b) {
        return std::tolower(a) == std::tolower(b);
      });
  return it != haystack.end();
}

void Plotter::processSessionData(SessionState &session) {
  session.measurements.clear();
  session.keysByDuration.clear();
  session.keysByAppearance.clear();
  session.measurementsPerSecond.resize(session.sessionData.size());
  std::vector<double> measurementsTimes(session.sessionData.size());
  std::unordered_map<uint64_t, measurement_element_t *> locationCache;
  constexpr size_t kProgressStride = 4096;
  for (size_t i = 0; i < session.sessionData.size(); i++) {
    const auto &row = session.sessionData[i];

    measurement_element_t *measPtr;
    auto cached = locationCache.find(row.locationId);
    if (cached == locationCache.end()) {
      measurement_element_t &meas = session.measurements[getLocation(row)];
      meas.function = row.function;
      meas.line = row.line;
      meas.path = row.path;
      meas.file = std::filesystem::path(row.path).filename();
      meas.name = row.name;
      measPtr = &meas;
      locationCache.emplace(row.locationId, measPtr);
    } else {
      measPtr = cached->second;
    }
    measurement_element_t &meas = *measPtr;

    meas.timeData.push_back({row.time, row.duration, row.threadId});
    if (meas.startAndDuration.time == -1) {
      meas.startAndDuration.time = row.time;
    }
    meas.meanDuration += row.duration;
    meas.startAndDuration.duration = row.time + row.duration;

    measurementsTimes[i] = row.time;

    if ((i % kProgressStride) == 0 || i + 1 == session.sessionData.size()) {
      session.progress = (double)(i + 1) / session.sessionData.size();
    }
  }

  if (measurementsTimes.empty()) {
    return;
  }

  std::sort(measurementsTimes.begin(), measurementsTimes.end());
  for (size_t i = 1; i < measurementsTimes.size() - 1; i++) {
    session.measurementsPerSecond[i].time = measurementsTimes[i];
    session.measurementsPerSecond[i].value =
        session.measurementsPerSecond[i - 1].value + measurementsTimes[i] -
        measurementsTimes[i - 1];
  }
  session.endTime = 0.0;
  for (auto &[loc, meas] : session.measurements) {
    std::sort(meas.timeData.begin(), meas.timeData.end(),
              [](const auto &a, const auto &b) { return a.time < b.time; });
    meas.displayLabel = meas.name + "\n" + meas.file + ":" +
                        std::to_string(meas.line) + "\n" + meas.function;
    meas.standardDeviation = 0.0;
    meas.meanFrequency = meas.timeData.size() / meas.startAndDuration.duration;
    meas.meanDuration /= meas.timeData.size();
    session.endTime = std::max(session.endTime, meas.timeData.back().time);

    std::vector<double> sortedDurations;
    sortedDurations.reserve(meas.timeData.size());
    for (const auto &timeData : meas.timeData) {
      sortedDurations.push_back(timeData.duration);
    }
    std::sort(sortedDurations.begin(), sortedDurations.end());
    meas.minDuration = sortedDurations.front();
    meas.maxDuration = sortedDurations.back();
    meas.p50Duration = percentileFromSorted(sortedDurations, 50.0);
    meas.p90Duration = percentileFromSorted(sortedDurations, 90.0);
    meas.p99Duration = percentileFromSorted(sortedDurations, 99.0);
    for (const auto &timeData : meas.timeData) {
      meas.standardDeviation +=
          std::pow(timeData.duration - meas.meanDuration, 2.0);
    }
    meas.standardDeviation =
        std::sqrt(meas.standardDeviation / meas.timeData.size());
    std::cout << getLocation(meas) << " => " << meas.meanDuration << " "
              << meas.standardDeviation << std::endl;
    session.keysByDuration.push_back(loc);
  }
  session.keysByAppearance = session.keysByDuration;
  std::sort(session.keysByDuration.begin(), session.keysByDuration.end(),
            [&](const auto &a, const auto &b) {
              const auto &elA = session.measurements[a];
              const auto &elB = session.measurements[b];
              return elA.meanDuration * elA.timeData.size() >
                     elB.meanDuration * elB.timeData.size();
            });
  std::sort(session.keysByAppearance.begin(), session.keysByAppearance.end(),
            [&](const std::string &a, const std::string &b) {
              const measurement_element_t &elA = session.measurements[a];
              const measurement_element_t &elB = session.measurements[b];
              return elA.startAndDuration.time < elB.startAndDuration.time;
            });
  for (size_t i = 0; i < session.keysByDuration.size(); i++) {
    session.measurements[session.keysByDuration[i]].durationSortedIndex = i;
  }
  for (size_t i = 0; i < session.keysByAppearance.size(); i++) {
    session.measurements[session.keysByAppearance[i]].appearanceSortedIndex = i;
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
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(borderColor), "%s:%" PRIu64 "",
                       element.file.c_str(), element.line);
    ImGui::Separator();
    ImGui::Text("Hits: %ld", element.timeData.size());
    ImGui::Text("Mean duration: %0.9f s", element.meanDuration);
    ImGui::Text("Mean frequency: %0.3f Hz", element.meanFrequency);
    ImGui::Text("Cumulative time: %0.9f s",
                element.meanDuration * element.timeData.size());
    ImGui::Separator();
    ImGui::Text("Min duration: %0.9f s", element.minDuration);
    ImGui::Text("p50 duration: %0.9f s", element.p50Duration);
    ImGui::Text("p90 duration: %0.9f s", element.p90Duration);
    ImGui::Text("p99 duration: %0.9f s", element.p99Duration);
    ImGui::Text("Max duration: %0.9f s", element.maxDuration);
    if (timeInstanceId != -1) {
      ImGui::Separator();
      ImGui::Text("Hit #: %ld", timeInstanceId);
      ImGui::Text("Time: %0.9f s", element.timeData[timeInstanceId].time);
      ImGui::Text("Duration: %0.9f s",
                  element.timeData[timeInstanceId].duration);
      ImGui::Text("Thread: %" PRIu64,
                  element.timeData[timeInstanceId].threadId);
    }
    ImGui::EndTooltip();
  }
}

void Plotter::plotTimeEvolution() {
  auto &measurements = primary.measurements;
  auto &endTime = primary.endTime;
  auto &measurementsPerSecond = primary.measurementsPerSecond;
  static double lowerThreshold = 0.0;
  ImGui::SetNextItemWidth(200);
  drawSortSelector();
  ImGui::SameLine();
  ImGui::Text("Skip samples with duration less than: ");
  ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
  ImGui::InputDouble("##skip_samples_every", &lowerThreshold);
  ImGui::SameLine();
  ImGui::Text("Search:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200);
  ImGui::InputText("##search_filter", &searchFilter);

  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0f;
  static ImPlotRect limits(0, endTime, 0, 0);
  const size_t maxAllowedSamples{5000};

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
                        ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_AutoFit);

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

      ImPlot::GetCurrentContext()->CurrentItems->ColormapIdx = 0;
      ImPlot::PushPlotClipRect();
      const auto mousePos = ImPlot::GetPlotMousePos();
      int row = -1;
      for (auto &[loc, meas] : measurements) {
        if (!searchFilter.empty() &&
            !containsCaseInsensitive(meas.displayLabel, searchFilter)) {
          continue;
        }
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

        size_t lastSamples = meas.lastFrameSamples;
        meas.lastFrameSamples = 0;
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

        const double searchMin = limits.Min().x - meas.maxDuration;
        auto seekIt = std::lower_bound(
            meas.timeData.begin(), meas.timeData.end(), searchMin,
            [](const measurement_element_t::time_and_duration &td,
               double value) { return td.time < value; });

        size_t startIdx = 0;
        size_t i = std::distance(meas.timeData.begin(), seekIt);
        size_t keepCounter = 0;
        for (; i < meas.timeData.size();
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

          if (skipEvery > 0) {
            bool skip = keepCounter % skipEvery == 0;
            keepCounter++;
            if (skip) {
              continue;
            }
          }
          ImVec2 rmin = ImPlot::PlotToPixels(
              ImPlotPoint(td.time, yIncrement * sortedRow));
          ImVec2 rmax = ImPlot::PlotToPixels(
              ImPlotPoint(td.time + td.duration, yIncrement * (sortedRow + 1)));
          ImPlotRect rect(rmin.x, rmax.x, rmin.y, rmax.y);
          ImPlot::GetPlotDrawList()->AddRectFilled(rmin, rmax, col);
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
        meas.lastFrameSamples = i - startIdx;
      }

      ImPlot::PopPlotClipRect();

      {
        double xDuration[2] = {0.0, endTime};
        double yLocation[2] = {0.0, 0.0};
        ImPlot::PlotLine("##Duration", xDuration, yLocation, 2);
      }

      row = 0;
      for (auto &[loc, meas] : measurements) {
        if (!searchFilter.empty() &&
            !containsCaseInsensitive(meas.displayLabel, searchFilter)) {
          continue;
        }
        int sortedRow = row;
        if (sortBy == (int)SortBy::Duration) {
          sortedRow = meas.durationSortedIndex;
        } else if (sortBy == (int)SortBy::Appearance) {
          sortedRow = meas.appearanceSortedIndex;
        }
        float sizeX = ImGui::CalcTextSize(meas.displayLabel.c_str()).x;
        ImPlot::PlotText(meas.displayLabel.c_str(), limits.Min().x,
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
    for (int i = 0; i < (int)previewFileLines.size(); i++) {

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
  auto &measurements = primary.measurements;
  auto &endTime = primary.endTime;
  static int opts = 0;

  ImGui::Text("Loaded path: %s", primary.loadedPath.c_str());

  if (ImGui::Button("Close session")) {
    primary.sessionCsvValid = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload")) {
    primary.shouldStartLoading = true;
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
                               "Counts", "Frequency", "Histogram"};
  ImGui::Combo("##Plot options", &opts, plotOptions, IM_ARRAYSIZE(plotOptions));
  ImGui::SameLine();
  ImGui::Text("Search:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(200);
  ImGui::InputText("##search_filter_bars", &searchFilter);

  constexpr int kHistogramOption = 5;
  if (opts == kHistogramOption) {
    static std::string selectedLocation;
    if (!measurements.empty() &&
        measurements.find(selectedLocation) == measurements.end()) {
      selectedLocation = measurements.begin()->first;
    }
    const measurement_element_t *selectedMeas =
        measurements.empty() ? nullptr : &measurements.at(selectedLocation);

    ImGui::SetNextItemWidth(400);
    const char *previewLabel = selectedMeas ? selectedMeas->name.c_str() : "No data";
    if (ImGui::BeginCombo("##histogram_location", previewLabel)) {
      for (auto &[loc, meas] : measurements) {
        if (!searchFilter.empty() &&
            !containsCaseInsensitive(meas.displayLabel, searchFilter)) {
          continue;
        }
        std::string itemLabel = meas.name + " (" + meas.file + ":" +
                                std::to_string(meas.line) + ")";
        bool isSelected = (loc == selectedLocation);
        if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
          selectedLocation = loc;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    auto histSize = ImGui::GetContentRegionAvail();
    if (ImPlot::BeginPlot("duration_histogram", histSize)) {
      ImPlot::SetupAxis(ImAxis_X1, "duration [s]");
      ImPlot::SetupAxis(ImAxis_Y1, "count");
      ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
      if (selectedMeas) {
        std::vector<double> durations;
        durations.reserve(selectedMeas->timeData.size());
        for (const auto &td : selectedMeas->timeData) {
          durations.push_back(td.duration);
        }
        ImPlot::PlotHistogram(selectedMeas->name.c_str(), durations.data(),
                              durations.size());
      }
      ImPlot::EndPlot();
    }
    return;
  }

  auto size = ImGui::GetContentRegionAvail();
  float yIncrement = 1.0F;
  if (ImPlot::BeginPlot("session", size)) {
    ImPlot::SetupAxis(ImAxis_Y1, "##measurements",
                      ImPlotAxisFlags_AutoFit);
    std::vector<double> yPosition(measurements.size());
    std::vector<double> bar(measurements.size());
    std::vector<double> std(measurements.size());
    std::vector<double> minVals(measurements.size());
    std::vector<double> p90Vals(measurements.size());
    std::vector<double> maxVals(measurements.size());

    int row = 0;
    for (auto &[loc, meas] : measurements) {
      if (!searchFilter.empty() &&
          !containsCaseInsensitive(meas.displayLabel, searchFilter)) {
        continue;
      }
      yPosition[row] = row;
      if (sortBy == (int)SortBy::Duration) {
        yPosition[row] = meas.durationSortedIndex;
      } else if (sortBy == (int)SortBy::Appearance) {
        yPosition[row] = meas.appearanceSortedIndex;
      }
      if (opts == 0) {
        bar[row] = meas.meanDuration;
        std[row] = meas.standardDeviation;
        minVals[row] = meas.minDuration;
        p90Vals[row] = meas.p90Duration;
        maxVals[row] = meas.maxDuration;
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
    yPosition.resize(row);
    bar.resize(row);
    std.resize(row);
    minVals.resize(row);
    p90Vals.resize(row);
    maxVals.resize(row);

    const char *name = plotOptions[opts];
    ImPlot::PlotBars(name, bar.data(), yPosition.data(), bar.size(),
                     yIncrement / 2.0, ImPlotBarsFlags_Horizontal);
    if (opts == 0) {
      ImPlot::PlotErrorBars("Standard deviation", bar.data(), yPosition.data(),
                            std.data(), bar.size(),
                            ImPlotErrorBarsFlags_Horizontal);
      ImPlot::PlotScatter("Min", minVals.data(), yPosition.data(),
                          minVals.size());
      ImPlot::PlotScatter("P90", p90Vals.data(), yPosition.data(),
                          p90Vals.size());
      ImPlot::PlotScatter("Max", maxVals.data(), yPosition.data(),
                          maxVals.size());
    }

    row = 0;
    auto pltMin = ImPlot::GetPlotLimits();
    for (auto &[loc, meas] : measurements) {
      if (!searchFilter.empty() &&
          !containsCaseInsensitive(meas.displayLabel, searchFilter)) {
        continue;
      }
      int sortedRow = row;
      if (sortBy == (int)SortBy::Duration) {
        sortedRow = meas.durationSortedIndex;
      } else if (sortBy == (int)SortBy::Appearance) {
        sortedRow = meas.appearanceSortedIndex;
      }
      float sizeX = ImGui::CalcTextSize(meas.displayLabel.c_str()).x;
      ImPlot::PlotText(meas.displayLabel.c_str(), pltMin.Min().x,
                       sortedRow * (yIncrement), ImVec2(sizeX / 2.0F, 0));
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
  auto &loadedPath = primary.loadedPath;
  auto &sessionData = primary.sessionData;
  auto &measurements = primary.measurements;
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
          out << "time;duration;thread;path;line;function;name\n";
          for (const auto &row : sessionData) {
            out << row.time << ";" << row.duration << ";" << row.threadId
                << ";" << row.path << ";" << row.line << ";" << row.function
                << ";" << row.name << "\n";
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
                 "mean frequency;hits;min duration;p50 duration;p90 duration;"
                 "p99 duration;max duration\n";
          for (const auto &[loc, meas] : measurements) {
            out << meas.name << ";" << meas.function << ";" << meas.file << ";"
                << meas.line << ";" << meas.meanDuration << ";"
                << meas.standardDeviation << ";" << meas.meanFrequency << ";"
                << meas.timeData.size() << ";" << meas.minDuration << ";"
                << meas.p50Duration << ";" << meas.p90Duration << ";"
                << meas.p99Duration << ";" << meas.maxDuration << "\n";
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

void Plotter::drawCompare() {
  if (comparison && comparison->shouldStartLoading) {
    startLoading(*comparison);
  }

  if (!comparison.has_value() ||
      (!comparison->sessionCsvValid && !comparison->loading)) {
    ImGui::Text(
        "Load a second session to compare against the currently loaded one.");
    std::string &path = KVP::getMutable("comparison path");
    if (drawPathPicker("comparison_path_picker", path)) {
      if (!comparison.has_value()) {
        comparison.emplace();
      }
      comparison->loadedPath = path;
      comparison->shouldStartLoading = true;
    }
    return;
  }

  if (comparison->loading) {
    ImGui::Text("Loading comparison session from %s",
                comparison->loadedPath.c_str());
    if (!comparison->sessionCsvValid) {
      ImGui::Text("Loading CSV file...");
    } else {
      ImGui::Text("Processing data...");
    }
    ImGui::ProgressBar(comparison->progress.load(), ImVec2(0.0f, 0.0f));
    return;
  }

  if (ImGui::Button("Unload comparison session")) {
    comparison.reset();
    return;
  }
  ImGui::SameLine();
  ImGui::Text("Comparing baseline (%s) against comparison (%s)",
              primary.loadedPath.c_str(), comparison->loadedPath.c_str());
  ImGui::Separator();

  struct CompareRow {
    const measurement_element_t *base;
    const measurement_element_t *comp;
  };
  std::vector<CompareRow> rows;
  rows.reserve(primary.measurements.size());
  std::vector<const measurement_element_t *> onlyBaseline;
  std::vector<const measurement_element_t *> onlyComparison;
  for (auto &[loc, meas] : primary.measurements) {
    auto it = comparison->measurements.find(loc);
    if (it != comparison->measurements.end()) {
      rows.push_back({&meas, &it->second});
    } else {
      onlyBaseline.push_back(&meas);
    }
  }
  for (auto &[loc, meas] : comparison->measurements) {
    if (primary.measurements.find(loc) == primary.measurements.end()) {
      onlyComparison.push_back(&meas);
    }
  }

  if (ImGui::BeginTable("compare_table", 6,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_ScrollY,
                        ImVec2(0, 300))) {
    ImGui::TableSetupColumn("Location");
    ImGui::TableSetupColumn("Baseline mean [s]");
    ImGui::TableSetupColumn("Comparison mean [s]");
    ImGui::TableSetupColumn("Delta [s]");
    ImGui::TableSetupColumn("Delta %");
    ImGui::TableSetupColumn("Hits (base/comp)");
    ImGui::TableHeadersRow();

    for (const auto &row : rows) {
      double deltaMean = row.comp->meanDuration - row.base->meanDuration;
      double deltaPct = row.base->meanDuration != 0.0
                             ? (deltaMean / row.base->meanDuration) * 100.0
                             : 0.0;
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(row.base->displayLabel.c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%.9f", row.base->meanDuration);
      ImGui::TableNextColumn();
      ImGui::Text("%.9f", row.comp->meanDuration);
      ImGui::TableNextColumn();
      ImGui::Text("%.9f", deltaMean);
      ImGui::TableNextColumn();
      ImVec4 color = deltaPct > 0 ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                  : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
      ImGui::TextColored(color, "%+.2f%%", deltaPct);
      ImGui::TableNextColumn();
      ImGui::Text("%zu / %zu", row.base->timeData.size(),
                  row.comp->timeData.size());
    }
    ImGui::EndTable();
  }

  if (!rows.empty()) {
    std::vector<double> yPosition(rows.size());
    std::vector<double> regressions(rows.size(), 0.0);
    std::vector<double> improvements(rows.size(), 0.0);
    for (size_t i = 0; i < rows.size(); i++) {
      double deltaMean =
          rows[i].comp->meanDuration - rows[i].base->meanDuration;
      double deltaPct = rows[i].base->meanDuration != 0.0
                             ? (deltaMean / rows[i].base->meanDuration) * 100.0
                             : 0.0;
      yPosition[i] = (double)i;
      if (deltaPct >= 0) {
        regressions[i] = deltaPct;
      } else {
        improvements[i] = deltaPct;
      }
    }
    auto chartSize = ImGui::GetContentRegionAvail();
    if (ImPlot::BeginPlot("compare_delta_chart", chartSize)) {
      ImPlot::SetupAxis(ImAxis_X1, "mean duration delta [%]");
      ImPlot::SetupAxis(ImAxis_Y1, "##location", ImPlotAxisFlags_AutoFit);
      ImPlot::SetNextFillStyle(ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
      ImPlot::PlotBars("Regression", regressions.data(), yPosition.data(),
                       regressions.size(), 0.6, ImPlotBarsFlags_Horizontal);
      ImPlot::SetNextFillStyle(ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
      ImPlot::PlotBars("Improvement", improvements.data(), yPosition.data(),
                       improvements.size(), 0.6, ImPlotBarsFlags_Horizontal);
      auto pltMin = ImPlot::GetPlotLimits();
      for (size_t i = 0; i < rows.size(); i++) {
        float sizeX = ImGui::CalcTextSize(rows[i].base->name.c_str()).x;
        ImPlot::PlotText(rows[i].base->name.c_str(), pltMin.Min().x,
                         yPosition[i], ImVec2(sizeX / 2.0f, 0));
      }
      ImPlot::EndPlot();
    }
  }

  if (!onlyBaseline.empty() || !onlyComparison.empty()) {
    ImGui::Separator();
    if (!onlyBaseline.empty()) {
      ImGui::Text("Only in baseline:");
      for (const auto *meas : onlyBaseline) {
        ImGui::BulletText("%s (%s:%" PRIu64 ")", meas->name.c_str(),
                          meas->file.c_str(), meas->line);
      }
    }
    if (!onlyComparison.empty()) {
      ImGui::Text("Only in comparison:");
      for (const auto *meas : onlyComparison) {
        ImGui::BulletText("%s (%s:%" PRIu64 ")", meas->name.c_str(),
                          meas->file.c_str(), meas->line);
      }
    }
  }
}
