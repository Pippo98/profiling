#pragma once

#include <atomic>
#include <map>
#include <thread>

#include "imgui.hpp"
#include "app_utils/app.hpp"
#include "csv.hpp"

extern ImFont *h1;
extern ImFont *h2;
extern ImFont *h3;
extern ImFont *text;

enum class SortBy {
	None,
	Duration,
	Appearance,
};

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
  std::string name;

  size_t durationSortedIndex;
  size_t appearanceSortedIndex;
};
inline std::string getLocation(const measurement_element_t &el) {
  return el.path + "(" + std::to_string(el.line) + "): " + el.function;
}
inline std::string getLocation(const session_row_t &el) {
  return std::string(el.path) + "(" + std::to_string(el.line) + "): " + std::string(el.function);
}

class Plotter : public App {
public:
	~Plotter();
protected:
  virtual void Draw();

private:
	void startLoading();
  void processSessionData();

  void plotTimeEvolution();
  void plotBars();
	void drawExportModal();

	void drawSortSelector();

	bool loading = false;
	bool shouldStartLoading = false;
  bool sessionCsvValid = false;
	bool exportModalOpen = false;

  std::vector<session_row_t> sessionData;
	std::map<uint64_t, id_map> locationIDMap;
  std::map<std::string, measurement_element_t> measurements;
  double endTime;
  std::string loadedPath;

  int previewFileLine;
  std::string previewFileName;
  std::vector<std::string> previewFileLines;

	std::atomic<float> progress = 0.0f;
	std::unique_ptr<std::thread> loadingThread;

  // list of measuresPerSeconds along the full log. measures how 
  // many rows per seconds there were.
  // Drops in this values means that nothing happened in those instances
  std::vector<time_value_pair_t<double>> measurementsPerSecond;
  std::vector<std::string> keysByDuration;
  std::vector<std::string> keysByAppearance;

	int sortBy = (int)SortBy::None;
};
