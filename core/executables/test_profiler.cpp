#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>

#include "defines.hpp"
#include "profiler/profiler.hpp"

using time_point = std::chrono::steady_clock::time_point;

long getDuration(time_point start, time_point end) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             end - start)
      .count();
}
long getDuration(time_point start) {
  return getDuration(start, std::chrono::steady_clock::now());
}

int main(void) {
  ProfilingSession::getGlobalInstace().initialize(PROJECT_PATH);

  constexpr int iterations = 8000;

  long durations[iterations];
  int a = 0;
  time_point total_start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; i++) {
    time_point t0, t1;
    time_point start = std::chrono::steady_clock::now();
    {
      static LocationID locId;
      MeasureScope scope(locId);
      t0 = std::chrono::steady_clock::now();
      for (int j = 0; j < 10000; j++) {
        a += rand();
      };
      t1 = std::chrono::steady_clock::now();
    }
    durations[i] = getDuration(start) - getDuration(t0, t1);
  }
  long total_duration = getDuration(total_start);
  std::cout << "value of dummy variable " << a << std::endl;
  long sum = 0;
  for (int i = 0; i < iterations; i++) {
    sum += durations[i];
  }
  printf("Time spent in MeasureScope: %0.9f s on a total of %0.9f s\n", sum / 1e9, total_duration / 1e9);
  std::cout << "Percentage: " << sum / (double)total_duration * 100
            << "%" << std::endl;

  // print mean in double
  double mean = 0;
  for (int i = 0; i < iterations; i++) {
    mean += durations[i];
  }
  mean /= iterations;
  printf("Mean: %0.9f s \n", mean / 1e9);

  return EXIT_SUCCESS;
}
