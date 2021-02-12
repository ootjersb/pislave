/*
 * Description: Tool to receive the data from the Itho device. Currently some
 * hardcoded actions are possible:
 * - Insert raw data into SQLite DB,
 * - Parse a single autotemp value and upload to Domoticz
 * - Parse datalog and print to screen
 *
 * compile:
 *  g++ -l pthread -l pigpio -lcurl -lsqlite3 -o pislave pislave.cpp
 *  datalogparser.cpp config.cpp conversion.cpp counterparser.cpp
 *
 * run: sudo ./pislave
 */
#include <curl/curl.h>
#include <pigpio.h>
#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "config.h"
#include "counterparser.h"
#include "datalogparser.h"

#define SIGTERM_MSG "SIGTERM received\n"

void closeSlave();
int getControlBits(int, bool);
void* registerCallback();
void writeToFile(char* filename, std::string data);
void processIncomingMessage(unsigned char* buffer, int length);
std::string bufferToReadableString(unsigned char* buffer, int length);
int GetURL(std::string myurl);
int PostURL(std::string myurl, std::string binary_data);
int insertlog(char* datalog);
void catch_sigterm();
void sig_term_handler(int signum, siginfo_t* info, void* ptr);

std::string directory;
const int slaveAddress = 0x40;  // <-- 0x40 is 7 bit address of 0x80
bsc_xfer_t xfer;                // Struct to control data flow
Config* config;
CounterParser* counter_parser_;
DatalogParser* p;

uint16_t stop = 0;  // 0 = running, 1 = stopping

int main(int argc, char* argv[]) {
  directory = argv[0];
  config = new Config(directory);
  p = new DatalogParser(config->DeviceType);
  counter_parser_ = new CounterParser(config->DeviceType);

  catch_sigterm();

  gpioInitialise();
  std::cout << "Initialized GPIOs" << std::endl;

  // Close old device (if any)
  xfer.control = getControlBits(slaveAddress,
                                false);  // To avoid conflicts when restarting
  bscXfer(&xfer);

  registerCallback();

  if (argc <= 1) {
    std::cout << "Running in service mode; infinite loop" << std::endl;

    while (stop == 0) {
      sleep(10);
    }

    std::cout << "Stopping service" << std::endl;

  } else {
    if (strcmp("--debug", argv[1]) == 0) {
      std::cout << "Running in debug mode" << std::endl;

      std::string strinput;
      std::string exit_command = "x";

      std::cout << "Enter a command (x=exit): ";

      while (strinput.compare(exit_command)) {
        getline(std::cin, strinput);
        std::cout << "You entered: " << strinput << std::endl;
      }

      std::cout << "stopping" << std::endl;

    } else {
      std::cout << "Unknown option, only --debug is support to run in console mode" << std::endl;
    }
  }

  closeSlave();

  delete p;
  delete config;
  return 0;
}

void sig_term_handler(int /*signum*/, siginfo_t* /*info*/, void* /*ptr*/) {
  write(STDERR_FILENO, SIGTERM_MSG, sizeof(SIGTERM_MSG));
  stop = 1;
}

void catch_sigterm() {
  static struct sigaction _sigact;

  memset(&_sigact, 0, sizeof(_sigact));
  _sigact.sa_sigaction = sig_term_handler;
  _sigact.sa_flags = SA_SIGINFO;

  sigaction(SIGTERM, &_sigact, NULL);
}

void ConvertCharToHex(unsigned char* buffer, char* converted, int bufferlength) {
  for (int i = 0; i < bufferlength; i++) {
    sprintf(&converted[i * 2], "%02X", buffer[i]);
  }
}

void simple_bsc_event_callback(int /*event*/, uint32_t /*tick*/) {
  bsc_xfer_t xfer;
  int status;

  xfer.control = getControlBits(slaveAddress, true);  // (slaveAddress << 16) | 0x305;
  status = bscXfer(&xfer);

  if (!status) {
    std::cerr << "process register failed." << std::endl;
  }

  if (xfer.rxCnt) {
    unsigned char* buffer = new unsigned char[1000];
    for (int i = 0; i < xfer.rxCnt; i++) buffer[i] = xfer.rxBuf[i];

    // write buffer and clear all
    processIncomingMessage(buffer, xfer.rxCnt);
  }
}

void* registerCallback() {
  int status;
  if (!eventSetFunc(PI_EVENT_BSC, simple_bsc_event_callback)) {
    xfer.control = getControlBits(slaveAddress, true);  // (slaveAddress << 16) | 0x305;
    status = bscXfer(&xfer);

    if (!status) {
      std::cerr << "process register failed." << std::endl;
    }
  }
  return NULL;
}

void closeSlave() {
  xfer.control = getControlBits(slaveAddress, false);
  bscXfer(&xfer);
  std::cout << "Closed slave." << std::endl;

  gpioTerminate();
  std::cout << "Terminated GPIOs." << std::endl;
}

int getControlBits(int address /* max 127 */, bool open) {
  /*
   Excerpt from http://abyz.me.uk/rpi/pigpio/cif.html#bscXfer regarding the
   control bits:event

   22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
   a  a  a  a  a  a  a  -  -  IT HC TF IR RE TE BK EC ES PL PH I2 SP EN

   Bits 0-13 are copied unchanged to the BSC CR register. See pages 163-165 of
   the Broadcom peripherals document for full details.

   aaaaaaa defines the I2C slave address (only relevant in I2C mode)
   IT  invert transmit status flags
   HC  enable host control
   TF  enable test FIFO
   IR  invert receive status flags
   RE  enable receive
   TE  enable transmit
   BK  abort operation and clear FIFOs
   EC  send control register as first I2C byte
   ES  send status register as first I2C byte
   PL  set SPI polarity high
   PH  set SPI phase high
   I2  enable I2C mode
   SP  enable SPI mode
   EN  enable BSC peripheral
   */

  // Flags like this:
  // 0b/*IT:*/0/*HC:*/0/*TF:*/0/*IR:*/0/*RE:*/0/*TE:*/0/*BK:*/0/*EC:*/0/*ES:*/0/*PL:*/0/*PH:*/0/*I2:*/0/*SP:*/0/*EN:*/0;
  int flags;
  if (open) {
    flags = /*RE:*/ (1 << 9) | /*TE:*/ (1 << 8) | /*I2:*/ (1 << 2) |
            /*EN:*/ (1 << 0);
  } else {
    // Close/Abort
    flags = /*BK:*/ (1 << 7) | /*I2:*/ (0 << 2) | /*EN:*/ (0 << 0);
  }
  // printf("flags=%d\n", flags);

  return (address << 16 /*= to the start of significant bits*/) | flags;
}

void UploadToDomoticz(std::string params) {
  std::stringstream domoticzUrl2;
  domoticzUrl2 << "http://" << config->DomoticzIP << "/json.htm?type=command&" << params;

  if (config->LogToConsole && config->DebugMode) {
    std::cout << "Calling " << domoticzUrl2.str() << std::endl;
  }
  GetURL(domoticzUrl2.str());
}

void UploadToInfluxDB(std::string params) {
  if (config->LogToConsole) {
    std::cout << "Sending data to influx: " << params << std::endl;
  }
  std::stringstream url;
  url << "http://" << config->influx_db_ip_ << ":" << config->influx_db_port_
      << "/write?db=" << config->influx_db_db_;

  PostURL(url.str(), params);
}

void UploadCounterToDomoticz(int idx, char* value) {
  std::stringstream param;
  param << "param=udevice&idx=" << idx << "&svalue=" << value;
  UploadToDomoticz(param.str());
}

void UploadSwitchToDomoticz(int idx, char* value) {
  std::stringstream param;
  if (std::strcmp(value, "1") == 0) {
    param << "param=switchlight&idx=" << idx << "&switchcmd=On";
  } else {
    param << "param=switchlight&idx=" << idx << "&switchcmd=Off";
  }

  UploadToDomoticz(param.str());
}

void UploadTemperatureToDomoticz(int idx, char* value) {
  std::stringstream param;
  param << "param=udevice&idx=" << idx << "&nvalue=0&svalue=" << value;

  UploadToDomoticz(param.str());
}

void UploadTemperatureToDomoticz(int idx, float value) {
  char sValue[50];
  sprintf(sValue, "%.2f", value);
  UploadTemperatureToDomoticz(idx, sValue);
}

void CheckChangeUploadSwitch(int idx, std::string label) {
  if (p->FieldChanged(label)) {
    if (config->LogToConsole)
      std::cout << label << ": " << p->FieldValue(label) << std::endl;

    if (config->LogToDomoticz)
      UploadSwitchToDomoticz(idx, p->FieldValue(label));
  }
}

void LogTemperatureFromLabel(int idx, std::string label, double max) {
  if (p->FieldChanged(label)) {
    double tempValue = atof(p->FieldValue(label));
    if ((max > 0.0) && (tempValue > max)) {
      std::cout << "Skipping bogus temperature " << tempValue << " for label " << label
                << std::endl;
      return;
    }

    if (config->LogToConsole)
      std::cout << label << ": " << p->FieldValue(label) << std::endl;

    if (config->LogToDomoticz)
      UploadTemperatureToDomoticz(idx, p->FieldValue(label));
  }
}

void LogFloatFromLabel(uint16_t /*idx*/, std::string label, std::string measurement) {
  float tempValue = std::stof(p->FieldValue(label));
  auto current_timestamp = std::chrono::system_clock::now();
  auto current_timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_timestamp.time_since_epoch())
          .count();

  std::stringstream influx_data;
  influx_data << measurement << "," << p->FieldCategory(label) << "=" << p->FieldTag(label)
              << " value=" << tempValue << " " << current_timestamp_ns;
  if (config->LogToConsole) {
    std::cout << label << ": " << p->FieldValue(label) << std::endl;
  }
  if (config->LogToInfluxDB) {
    UploadToInfluxDB(influx_data.str());
  }
}

void LogUint16FromLabel(uint16_t /*idx*/, std::string label, std::string measurement) {
  uint16_t tempValue = static_cast<uint16_t>(std::stoi(p->FieldValue(label)));
  auto current_timestamp = std::chrono::system_clock::now();
  auto current_timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_timestamp.time_since_epoch())
          .count();

  std::stringstream influx_data;
  influx_data << measurement << "," << p->FieldCategory(label) << "=" << p->FieldTag(label)
              << " value=" << tempValue << " " << current_timestamp_ns;
  if (config->LogToConsole) {
    std::cout << label << ": " << p->FieldValue(label) << std::endl;
  }
  if (config->LogToInfluxDB) {
    UploadToInfluxDB(influx_data.str());
  }
}

void LogUint16FromLabelCounter(uint16_t /*idx*/, std::string label, std::string measurement) {
  uint16_t tempValue = static_cast<uint16_t>(std::stoi(counter_parser_->FieldValue(label)));
  auto current_timestamp = std::chrono::system_clock::now();
  auto current_timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_timestamp.time_since_epoch())
          .count();

  std::stringstream influx_data;
  influx_data << measurement << "," << counter_parser_->FieldCategory(label) << "="
              << counter_parser_->FieldTag(label) << " value=" << tempValue << " "
              << current_timestamp_ns;
  if (config->LogToConsole) {
    std::cout << label << ": " << counter_parser_->FieldValue(label) << std::endl;
  }
  if (config->LogToInfluxDB) {
    UploadToInfluxDB(influx_data.str());
  }
}

void LogUint8FromLabel(uint16_t /*idx*/, std::string label, std::string measurement) {
  uint8_t tempValue = static_cast<uint8_t>(std::stoi(p->FieldValue(label)));
  auto current_timestamp = std::chrono::system_clock::now();
  auto current_timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_timestamp.time_since_epoch())
          .count();

  std::stringstream influx_data;
  influx_data << measurement << "," << p->FieldCategory(label) << "=" << p->FieldTag(label)
              << " value=" << static_cast<uint16_t>(tempValue) << " " << current_timestamp_ns;
  if (config->LogToConsole) {
    std::cout << label << ": " << p->FieldValue(label) << std::endl;
  }
  if (config->LogToInfluxDB) {
    UploadToInfluxDB(influx_data.str());
  }
}

void LogBoolFromLabel(uint16_t /*idx*/, std::string label, std::string measurement) {
  bool tempValue = static_cast<bool>(std::stoi(p->FieldValue(label)));
  auto current_timestamp = std::chrono::system_clock::now();
  auto current_timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_timestamp.time_since_epoch())
          .count();

  std::stringstream influx_data;
  influx_data << measurement << "," << p->FieldCategory(label) << "=" << p->FieldTag(label)
              << " value=" << tempValue << " " << current_timestamp_ns;
  if (config->LogToConsole) {
    std::cout << label << ": " << p->FieldValue(label) << std::endl;
  }
  if (config->LogToInfluxDB) {
    UploadToInfluxDB(influx_data.str());
  }
}

void LogDecimalFromLabel(int idx, std::string label) {
  if (p->FieldChanged(label)) {
    if (config->LogToConsole)
      std::cout << label << ": " << p->FieldValue(label) << std::endl;

    if (config->LogToDomoticz)
      UploadTemperatureToDomoticz(idx, p->FieldValue(label));
  }
}

void LogCounterFromLabel(int idx, std::string label) {
  if (p->FieldChanged(label)) {
    if (config->LogToConsole)
      std::cout << label << ": " << p->FieldValue(label) << std::endl;

    if (config->LogToDomoticz)
      UploadCounterToDomoticz(idx, p->FieldValue(label));
  }
}

void ParseDatalogAutotemp(unsigned char* buffer, int length) {
  if (!p->ParseWithHeader(buffer, length)) {
    if (config->LogToConsole)
      std::cout << "Failed to parse header/checksum" << std::endl;
    return;
  }

  // Outside temperature is now logged by WPU reader
  // UploadTemperatureToDomoticz(398, outsideTemp);
  LogTemperatureFromLabel(592, "Ruimte 1 temp", 50.0);
  LogTemperatureFromLabel(576, "Ruimte 3 temp", 50.0);
  LogTemperatureFromLabel(577, "Ruimte 4 temp", 50.0);
  LogTemperatureFromLabel(578, "Ruimte 2 temp", 50.0);
  LogTemperatureFromLabel(579, "Ruimte 5 temp", 50.0);
  LogTemperatureFromLabel(580, "Ruimte 6 temp", 50.0);
  LogTemperatureFromLabel(581, "Gewenst vermogen", 100.0);
  LogTemperatureFromLabel(598, "Rest cyclustijd", 2000.0);
  LogTemperatureFromLabel(599, "Comm Ruimte A", 0.0);
  LogTemperatureFromLabel(595, "Comm Ruimte B", 0.0);
  LogTemperatureFromLabel(594, "Comm Ruimte C", 0.0);
  LogTemperatureFromLabel(584, "Comm Ruimte D", 0.0);
  LogTemperatureFromLabel(596, "Comm Ruimte E", 0.0);
  LogTemperatureFromLabel(597, "Comm Ruimte F", 0.0);
  LogTemperatureFromLabel(585, "Ruimte 1 setp", 50.0);
  LogTemperatureFromLabel(586, "Ruimte 3 setp", 50.0);
  LogTemperatureFromLabel(587, "Ruimte 4 setp", 50.0);
  LogTemperatureFromLabel(588, "Ruimte 2 setp", 50.0);
  LogTemperatureFromLabel(589, "Ruimte 5 setp", 50.0);
  LogTemperatureFromLabel(590, "Ruimte 6 setp", 50.0);
  LogTemperatureFromLabel(593, "Ruimte 1 vermogen %", 100.0);
  LogTemperatureFromLabel(600, "Ruimte 3 vermogen %", 100.0);
  LogTemperatureFromLabel(601, "Ruimte 4 vermogen %", 100.0);
  LogTemperatureFromLabel(602, "Ruimte 2 vermogen %", 100.0);
  LogTemperatureFromLabel(603, "Ruimte 5 vermogen %", 100.0);
  LogTemperatureFromLabel(604, "Ruimte 6 vermogen %", 100.0);
  LogCounterFromLabel(591, "Foutcode");
}

void ParseDatalogHeatPump(unsigned char* buffer, int length) {
  if (!p->ParseWithHeader(buffer, length)) {
    return;
  }

  LogFloatFromLabel(0, "Buitentemperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(1, "Boiler hoog", config->influx_db_datalog_table_);
  LogFloatFromLabel(2, "Boiler laag", config->influx_db_datalog_table_);
  LogFloatFromLabel(3, "Verdamper temperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(4, "Zuiggas temperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(5, "Persgas temperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(6, "Vloeistof temperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(7, "Van bron", config->influx_db_datalog_table_);
  LogFloatFromLabel(8, "Naar bron", config->influx_db_datalog_table_);
  LogFloatFromLabel(9, "CV aanvoer", config->influx_db_datalog_table_);
  LogFloatFromLabel(10, "CV retour", config->influx_db_datalog_table_);
  LogFloatFromLabel(11, "Druksensor", config->influx_db_datalog_table_);
  LogFloatFromLabel(12, "Stroom Compressor", config->influx_db_datalog_table_);
  LogFloatFromLabel(13, "Stroom E-element", config->influx_db_datalog_table_);

  LogUint16FromLabel(19, "Flow sensor bron", config->influx_db_datalog_table_);

  LogUint8FromLabel(21, "Snelheid cv pomp (%)", config->influx_db_datalog_table_);
  LogUint8FromLabel(22, "Snelheid bron pomp (%)", config->influx_db_datalog_table_);
  LogUint8FromLabel(23, "Snelheid boiler pomp (%)", config->influx_db_datalog_table_);

  LogBoolFromLabel(24, "Compressor aan/uit", config->influx_db_datalog_table_);
  LogBoolFromLabel(27, "Elektrisch element aan/uit", config->influx_db_datalog_table_);
  //  LogBoolFromLabel(29, "Fout aanwezig (0=J, 1=N)");

  LogFloatFromLabel(32, "Kamertemperatuur", config->influx_db_datalog_table_);
  LogFloatFromLabel(33, "Kamertemperatuur setpoint", config->influx_db_datalog_table_);
  LogUint8FromLabel(34, "Warmtevraag", config->influx_db_datalog_table_);
  LogUint8FromLabel(35, "State (0=init,1=uit,2=CV,3=boiler,4=vrijkoel,5=ontluchten)",
                    config->influx_db_datalog_table_);
  LogUint8FromLabel(36, "Substatus (255=geen)", config->influx_db_datalog_table_);
  LogUint8FromLabel(42, "Fout gevonden (foutcode)", config->influx_db_datalog_table_);
  LogUint8FromLabel(97, "Warmtevraag totaal", config->influx_db_datalog_table_);
  //  // LogFloatFromLabel(606, "Vrijkoelen interval (sec)", 0.0);
  //  LogDecimalFromLabel(55, "E-consumption stand-by");
  //  LogDecimalFromLabel(56, "E-consumption heating");
  //  LogDecimalFromLabel(57, "E-consumption DHW");
  //  LogDecimalFromLabel(58, "E-consumption cooling");
}

void ParseCountersHeatPump(unsigned char* buffer, int length) {
  if (!counter_parser_->ParseWithHeader(buffer, length)) {
    return;
  }

  LogUint16FromLabelCounter(1, "bedrijf cv pomp", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(2, "bedrijf bron pomp", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(3, "bedrijf boiler pomp", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(4, "bedrijf compressor", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(5, "bedrijf elektrisch element", config->influx_db_counter_table_);
  // 6 - 10
  LogUint16FromLabelCounter(6, "cv bedrijf", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(7, "boiler bedrijf", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(8, "vrijkoel bedrijf", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(9, "bedrijf", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(10, "cv pomp starts", config->influx_db_counter_table_);
  // 11 - 15
  LogUint16FromLabelCounter(11, "bron pomp starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(12, "boiler pomp starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(13, "compressor starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(14, "elektrisch element starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(15, "cv starts", config->influx_db_counter_table_);
  // 16 - 20
  LogUint16FromLabelCounter(16, "boiler starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(17, "vrijkoel starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(18, "systeem starts", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(19, "bedrijf DHW element", config->influx_db_counter_table_);
  LogUint16FromLabelCounter(20, "DHW E-element starts", config->influx_db_counter_table_);
}

void WriteMessageToSQLLite(unsigned char* buffer, int length) {
  // Write the message to SQL Lite
  char* bufferhex = new char[2003];  // plus 2 for 80 destination and 1 for \0
  sprintf(bufferhex, "80");
  ConvertCharToHex(buffer, bufferhex + 2, length);
  insertlog(bufferhex);
}

void processIncomingMessage(unsigned char* buffer, int length) {
  if (config->LogToSqlLite)
    WriteMessageToSQLLite(buffer, length);

  // Then process it further
  char filename[128];
  std::string message_name;

  if (buffer[1] == 0xA4 && buffer[2] == 0x10)  // ophalen setting
  {
    std::cout << "Processing setting message" << std::endl;

    message_name = "OphalenSetting";
    sprintf(filename, "/home/pi/pislave/data/%02X%02X-%02X.txt", buffer[1], buffer[2],
            buffer[17 + 5]);

  } else if (buffer[1] == 0xC0 && buffer[2] == 0x30)  // ophalen config
  {
    std::cout << "Processing config message" << std::endl;

    message_name = "OphalenConfig";
    sprintf(filename, "/home/pi/pislave/data/%02X%02X-%02X%02X%02X.txt", buffer[1], buffer[2],
            buffer[5 + 0], buffer[5 + 1], buffer[5 + 2]);

  } else if (buffer[1] == 0xC2 && buffer[2] == 0x10)  // Counters
  {
    std::cout << "Processing counters message" << std::endl;
    message_name = "Counters";
    sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]);

    if (config->DeviceType == DatalogParser::kDeviceIdHeatpump) {
      ParseCountersHeatPump(buffer, length);
    }

  } else if (buffer[1] == 0xA4 && buffer[2] == 0x01)  // datalog
  {
    std::cout << "Processing datalog message" << std::endl;

    message_name = "Datalog";
    sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1],
            buffer[2]);  // TODO: add timestamp??

    if (config->DeviceType == DatalogParser::kDeviceIdHeatpump) {
      ParseDatalogHeatPump(buffer, length);
    }

    if (config->DeviceType == DatalogParser::kDeviceIdAutotemp) {
      ParseDatalogAutotemp(buffer, length);
    }

  } else {
    std::cout << "Processing unknown message" << std::endl;

    message_name = "Unknown";
    sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]);

    if (config->LogUnknownTypes) {
      std::cout << "Writing message " << message_name << " to " << filename << std::endl;

      std::string data = bufferToReadableString(buffer, length);
      writeToFile(filename, data);
    }
  }

  std::string data = bufferToReadableString(buffer, length);
  if (config->LogToConsole && config->LogMessageToConsole) {
    std::cout << data << std::endl;
  }
  if (config->LogToFile) {
    if (config->LogToConsole) {
      std::cout << "Writing message " << message_name << " to " << filename << std::endl;
    }
    writeToFile(filename, data);
  }
}

std::string bufferToReadableString(unsigned char* buffer, int length) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  ss << "[0x80";
  for (uint16_t i = 0; i < length; ++i) {
    ss << " 0x" << std::uppercase << std::setw(2) << static_cast<uint16_t>(buffer[i]);
  }
  ss << "]";
  return ss.str();
}

void writeToFile(char* filename, std::string data) {
  std::ofstream myfile;
  myfile.open(filename);
  myfile << data;
  myfile.close();
}

typedef struct string_buffer_s {
  char* ptr;
  size_t len;
} string_buffer_t;

static void string_buffer_initialize(string_buffer_t* sb) {
  sb->len = 0;
  sb->ptr = (char*)malloc(sb->len + 1);
  sb->ptr[0] = '\0';
}

static void string_buffer_finish(string_buffer_t* sb) {
  free(sb->ptr);
  sb->len = 0;
  sb->ptr = NULL;
}

static size_t string_buffer_callback(void* buf, size_t size, size_t nmemb, void* data) {
  string_buffer_t* sb = (string_buffer_t*)data;
  size_t new_len = sb->len + size * nmemb;

  sb->ptr = (char*)realloc(sb->ptr, new_len + 1);

  memcpy(sb->ptr + sb->len, buf, size * nmemb);

  sb->ptr[new_len] = '\0';
  sb->len = new_len;

  return size * nmemb;
}

static size_t header_callback(char* buf, size_t size, size_t nmemb, void* data) {
  return string_buffer_callback(buf, size, nmemb, data);
}

static size_t write_callback(void* buf, size_t size, size_t nmemb, void* data) {
  return string_buffer_callback(buf, size, nmemb, data);
}

int GetURL(std::string myurl) {
  CURL* curl;
  CURLcode res;
  string_buffer_t strbuf;

  string_buffer_initialize(&strbuf);

  curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "Fatal: curl_easy_init() error.\n");
    string_buffer_finish(&strbuf);
    return EXIT_FAILURE;
  }

  curl_easy_setopt(curl, CURLOPT_URL, myurl.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strbuf);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &strbuf);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
    string_buffer_finish(&strbuf);

    return EXIT_FAILURE;
  }

  // printf( "%s\n\n", strbuf.ptr );

  curl_easy_cleanup(curl);
  string_buffer_finish(&strbuf);

  return EXIT_SUCCESS;
}

int PostURL(std::string myurl, std::string binary_data) {
  CURL* curl;
  CURLcode res;
  //  string_buffer_t strbuf;
  //
  //  string_buffer_initialize(&strbuf);

  curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "Fatal: curl_easy_init() error.\n");
    //    string_buffer_finish(&strbuf);
    return EXIT_FAILURE;
  }

  curl_easy_setopt(curl, CURLOPT_URL, myurl.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, binary_data.c_str());

  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
    //    string_buffer_finish(&strbuf);

    return EXIT_FAILURE;
  }

  curl_easy_cleanup(curl);
  //  string_buffer_finish(&strbuf);

  return EXIT_SUCCESS;
}

int callback(void* /*NotUsed*/, int argc, char** argv, char** azColName) {
  for (uint16_t i = 0; i < argc; i++) {
    std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL");
    // printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  std::cout << std::endl;
  return 0;
}

int executesql(char* sql) {
  sqlite3* db;
  char* zErrMsg = 0;
  int rc;

  rc = sqlite3_open("sqlite.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return (1);
  }
  std::cout << "Executing " << sql << std::endl;
  // printf("Executing %s\n", sql);
  rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  return rc;
}

int insertlog(char* datalog) {
  const char* sqltemplate = "insert into datalog(moment, data) values (current_timestamp, '%s')";
  char* statement = (char*)malloc(std::strlen(datalog) + std::strlen(sqltemplate) + 1);
  sprintf(statement, sqltemplate, datalog);
  int rc = executesql(statement);
  free(statement);
  return rc;
}
