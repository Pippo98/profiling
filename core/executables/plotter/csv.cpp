#include "csv.hpp"
#include "defines.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data) {
  std::ifstream locationIDMapFile(path + SESSION_ID_MAP_FILENAME,
                                  std::fstream::in);
  if (!locationIDMapFile.is_open()) {
    return false;
  }
  FILE *csv = fopen((path + SESSION_FILENAME).c_str(), "rb");
  if (!csv) {
    return false;
  }

  struct id_map {
    uint64_t id;
    std::string file;
    int line;
    std::string function;
  };
  std::map<uint64_t, id_map> locationIDMap;

  std::string line;
  while (std::getline(locationIDMapFile, line)) {
    std::stringstream ss(line);
    id_map el;
    std::string idStr, lineStr;

    std::getline(ss, el.file, ',');
    std::getline(ss, lineStr, ',');
    std::getline(ss, el.function, ',');
    std::getline(ss, idStr);

    try {
      el.id = std::stoi(idStr);
      el.line = std::stoi(lineStr);
      locationIDMap[el.id] = el;
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error: Invalid data format in the CSV file!" << std::endl;
    }
  }

  session_row_binary_t ser;
  data.clear();
  while (fread(&ser, sizeof(ser), 1, csv)) {
    session_row_t row;

    row.file = locationIDMap[ser.location_id].file;
    row.line = locationIDMap[ser.location_id].line;
    row.function = locationIDMap[ser.location_id].function;
    row.time = ser.time;
    row.duration = ser.duration;

    data.push_back(row);
  }
  return true;
}
