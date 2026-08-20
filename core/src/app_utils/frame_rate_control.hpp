#pragma once

#include "imgui.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <thread>

class FrameRateControl {
public:
  FrameRateControl(double maxFPS, double idleFPS, double holdTime = 0.5,
                   double maxToIdleTime = 1.0)
      : minDT(1.0 / maxFPS), idleDT(1.0 / idleFPS), holdTime(holdTime),
        minToIdleTime(maxToIdleTime) {
    if (minDT >= idleDT) {
      throw std::logic_error("Idle FPS must be lower than idle FPS");
    }
    holdTime = std::max(holdTime, 3.0 * idleDT);
    tLastInput = clk.now();
  }
  void keepHighFPS() { tLastInput = clk.now(); }
  void start() {
    t0 = clk.now();
    bool hadEvent = false;
    auto &io = ImGui::GetIO();
    hadEvent |= io.MouseDelta.x != 0.0;
    hadEvent |= io.MouseDelta.y != 0.0;
    hadEvent |= io.MouseWheel != 0.0;
    hadEvent |= io.KeyCtrl || io.KeyAlt || io.KeyShift || io.KeySuper;
    for (size_t i = 0; i < ImGuiKey_NamedKey_COUNT; i++) {
      hadEvent |= io.KeysData[i].Down;
      hadEvent |= io.KeysData[i].AnalogValue > 0.0;
    }
    if (hadEvent) {
      tLastInput = t0;
    }
  }
  void maybeWait() {
    using namespace std::chrono;
    double dt = duration_cast<microseconds>(clk.now() - t0).count() / 1e6;

    double lastInputDT =
        duration_cast<microseconds>(clk.now() - tLastInput).count() / 1e6;
    double t =
        std::clamp(lastInputDT - holdTime, 0.0, minToIdleTime) / minToIdleTime;

    double targetDT = minDT + t * (idleDT - minDT);
    if (dt < targetDT) {
      std::this_thread::sleep_for(
          microseconds(static_cast<long long>((targetDT - dt) * 1e6)));
    }
  }

private:
  double minDT;
  double idleDT;
  double holdTime;
  double minToIdleTime;

  std::chrono::high_resolution_clock clk;
  std::chrono::time_point<std::chrono::high_resolution_clock> t0;
  std::chrono::time_point<std::chrono::high_resolution_clock> tLastInput;
};
