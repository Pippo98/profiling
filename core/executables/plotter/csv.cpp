#include "csv.hpp"
#include "defines.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

bool ReadSessionCSV(const std::string &path, std::vector<session_row_t> &data) {
  std::ifstream csv(path + SESSION_NAME, std::fstream::in);
  if (!csv.is_open()) {
    return false;
  }

  data.clear();
  std::string line;
  while (std::getline(csv, line)) {
    session_row_t row;
    std::stringstream ss(line);
    std::string timeStr, location, durationStr;

    std::getline(ss, timeStr, ',');
    std::getline(ss, location, ',');
    std::getline(ss, durationStr);

    try {
      row.time = std::stod(timeStr);
      row.location = location;
      row.duration = std::stod(durationStr);
      data.push_back(row);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error: Invalid data format in the CSV file!" << std::endl;
    }
  }
  return true;
}
