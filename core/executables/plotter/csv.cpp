#include "csv.hpp"
#include "defines.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data,
                    std::map<uint64_t, id_map> &locationIDMap,
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

  session_row_binary_t ser;
  data.clear();
  data.reserve(csvSize / sizeof(ser));
  while (fread(&ser, sizeof(ser), 1, csv)) {
    data.emplace_back(session_row_t{ser.time, ser.duration,
                                    locationIDMap[ser.location_id].path,
                                    locationIDMap[ser.location_id].line,
                                    locationIDMap[ser.location_id].function,
                                    locationIDMap[ser.location_id].name});
    progress = (float)data.size() * sizeof(ser) / csvSize;
  }
  fclose(csv);
  return true;
}
