#include "csv.hpp"
#include "profiler/profiler.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data,
                    std::unordered_map<uint64_t, id_map> &locationIDMap,
                    std::atomic<float> &progress) {
  std::ifstream locationIDMapFile(path + SESSION_ID_MAP_FILENAME,
                                  std::fstream::in);
  if (!locationIDMapFile.is_open()) {
    return false;
  }
  FILE *csv = fopen((path + SESSION_FILENAME).c_str(), "rb");
  if (!csv) {
    return false;
  }

	locationIDMap.clear();
  std::string line;
  while (std::getline(locationIDMapFile, line)) {
    std::stringstream ss(line);
    id_map el;
    std::string idStr, lineStr;

    std::getline(ss, el.path, ';');
    std::getline(ss, lineStr, ';');
    std::getline(ss, el.function, ';');
    std::getline(ss, el.name, ';');
    std::getline(ss, idStr);

    try {
      el.id = std::stoull(idStr);
      el.line = std::stoi(lineStr);
      locationIDMap[el.id] = el;
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error: Invalid data format in the CSV file!" << std::endl;
    }
  }
  locationIDMapFile.close();

  size_t csvSize = 0;
  fseek(csv, 0, SEEK_END);
  csvSize = ftell(csv);
  fseek(csv, 0, SEEK_SET);

  const size_t recordCount = csvSize / sizeof(session_row_binary_t);
  std::vector<session_row_binary_t> rawRecords(recordCount);
  const size_t readCount =
      fread(rawRecords.data(), sizeof(session_row_binary_t), recordCount, csv);
  fclose(csv);
  rawRecords.resize(readCount);

  data.clear();
  data.reserve(readCount);
  constexpr size_t kProgressStride = 4096;
  for (size_t i = 0; i < readCount; i++) {
    const session_row_binary_t &ser = rawRecords[i];
    const id_map &loc = locationIDMap[ser.location_id];
    data.emplace_back(session_row_t{ser.time / 1e9, ser.duration / 1e9,
                                    loc.path, loc.line, loc.function,
                                    loc.name});
    if ((i % kProgressStride) == 0 || i + 1 == readCount) {
      progress = (float)(i + 1) / readCount;
    }
  }
  return true;
}
