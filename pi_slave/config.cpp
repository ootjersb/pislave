#include "config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Config::Config(std::string directory) : configdir(directory) { ReadConfigFile(); }

void Config::ReadConfigFile() {
  size_t slash_pos = configdir.rfind("/");
  std::string configfile = configdir.substr(0, slash_pos) + "/config.txt";
  std::cout << "Reading configuration file " << configfile << std::endl;

  std::ifstream infile(configfile);
  std::string line;
  while (std::getline(infile, line)) {
    if (line.size() > 1 && line.substr(0, 1) == "#")  // comment line
      continue;

    size_t pos = line.find('=', 0);
    if (pos > 0 && pos != 4294967295) {
      ParseOptionLine(line, pos);
    }
  }
}

void Config::ParseOptionLine(std::string& line, const size_t& pos) {
  std::string option = line.substr(0, pos);
  std::string value = line.substr(pos + 1, line.size() - pos - 1);

  if (option == "DeviceType") {
    std::cout << "DeviceType=" << value << std::endl;
    DeviceType = std::stoi(value);

  } else if (option == "DomoticzIP") {
    std::cout << "DomoticzIP=" << value << std::endl;
    DomoticzIP = value;

    // InfluxDB settings
  } else if (option == "InfluxdbIP") {
    influx_db_ip_ = value;
    std::cout << "InfluxdbIP=" << influx_db_ip_ << std::endl;

  } else if (option == "InfluxdbPort") {
    influx_db_port_ = static_cast<uint16_t>(std::stoi(value));
    std::cout << "InfluxdbPort=" << influx_db_port_ << std::endl;

  } else if (option == "InfluxdbDB") {
    std::cout << "InfluxdbDB=" << value << std::endl;
    influx_db_db_ = value;

    // Other settings
  } else if (option == "LogToFile") {
    LogToFile = value == "true";

  } else if (option == "LogToSqlLite") {
    LogToSqlLite = value == "true";

  } else if (option == "LogToDomoticz") {
    LogToDomoticz = value == "true";

  } else if (option == "LogToInfluxDB") {
    LogToInfluxDB = value == "true";

  } else if (option == "LogToConsole") {
    LogToConsole = value == "true";

  } else if (option == "LogMessageToConsole") {
    LogMessageToConsole = value == "true";

  } else if (option == "LogUnknownTypes") {
    LogUnknownTypes = value == "true";

  } else if (option == "DebugMode") {
    DebugMode = value == "true";
  }
}
