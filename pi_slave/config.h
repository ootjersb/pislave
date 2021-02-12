#pragma once
#include <string>

class Config {
 public:
  uint16_t DeviceType{13};

  std::string DomoticzIP{"localhost"};

  std::string influx_db_ip_{"localhost"};
  uint16_t influx_db_port_{8086};
  std::string influx_db_db_{"itho_wpu"};
  std::string influx_db_datalog_table_{"datalog"};
  std::string influx_db_counter_table_{"counters"};

  bool LogToFile{false};
  bool LogToSqlLite{false};
  bool LogToDomoticz{false};
  bool LogToInfluxDB{false};
  bool LogToConsole{true};
  bool LogMessageToConsole{false};
  bool LogUnknownTypes{false};

  bool DebugMode{false};

  Config(std::string directory);
  void ReadConfigFile();
  void ParseOptionLine(std::string& line, const size_t& pos);

 private:
  std::string configdir;
};
