#ifndef DATALOGPARSER_H
#define DATALOGPARSER_H

#include <algorithm>
#include <cstdint>

#include "field.h"

class DatalogParser {
 public:
  DatalogParser(uint16_t device);
  bool ParseWithHeader(unsigned char* buffer, int length);
  void Parse(unsigned char* buffer, int length);
  void Parse(char* hexstring);
  char* FieldValue(uint16_t index);
  char* FieldValue(std::string label);
  bool FieldChanged(uint16_t index);
  bool FieldChanged(std::string label);
  const std::string FieldCategory(int index);
  const std::string FieldCategory(std::string label);
  const std::string FieldTag(int index);
  const std::string FieldTag(std::string label);

  uint16_t fieldCount;

  static constexpr uint16_t kDeviceIdHeatpump = 13;
  static constexpr uint16_t kDeviceIdAutotemp = 15;

 private:
  int char2int(char input);
  void hex2bin(const char* src, unsigned char* target);
  int GetFieldIndex(std::string label);

  static constexpr uint16_t kFieldCountHeatpump = 35;
  static constexpr uint16_t kFieldCountAutotemp = 37;

  char errorMessage[100];

  char fieldValues[std::max(kFieldCountHeatpump, kFieldCountHeatpump)]
                  [50];  // only the bigger of the two taken here
  char previousValues[std::max(kFieldCountHeatpump, kFieldCountHeatpump)]
                     [50];  // only the bigger of the two taken here

  Field* config;

  Field warmtepompConfig[kFieldCountHeatpump]{
      // Offset  - datatype - label - category - tag
      //  1 -  5
      {0, FieldType::SignedIntDec2, "Buitentemperatuur", "temperature", "outdoor"},
      {2, FieldType::SignedIntDec2, "Boiler laag", "temperature", "boiler_low"},
      {4, FieldType::SignedIntDec2, "Boiler hoog", "temperature", "boiler_high"},
      {6, FieldType::SignedIntDec2, "Verdamper temperatuur", "temperature", "evaporator"},
      {8, FieldType::SignedIntDec2, "Zuiggas temperatuur", "temperature", "suction_gas"},
      //  6 - 10
      {10, FieldType::SignedIntDec2, "Persgas temperatuur", "temperature", "compressed_gas"},
      {12, FieldType::SignedIntDec2, "Vloeistof temperatuur", "temperature", "liquid"},
      {14, FieldType::SignedIntDec2, "Naar bron", "temperature", "to_source"},
      {16, FieldType::SignedIntDec2, "Van bron", "temperature", "from_source"},
      {18, FieldType::SignedIntDec2, "CV aanvoer", "temperature", "ch_supply"},
      // 11 - 15
      {20, FieldType::SignedIntDec2, "CV retour", "temperature", "ch_return"},
      {22, FieldType::SignedIntDec2, "Druksensor", "pressure", "ch"},
      {24, FieldType::SignedIntDec2, "Stroom Compressor", "current", "compressor"},
      {26, FieldType::SignedIntDec2, "Stroom E-element", "current", "e_element"},
      // 16 - 20
      {33, FieldType::UnsignedInt, "Flow sensor bron", "flow", "source"},
      // 21 - 25
      {36, FieldType::Byte, "Snelheid cv pomp (%)", "speed", "pump_ch"},
      {37, FieldType::Byte, "Snelheid bron pomp (%)", "speed", "pump_source"},
      {38, FieldType::Byte, "Snelheid boiler pomp (%)", "speed", "pump_dhw"},
      // 26 - 30
      {41, FieldType::Byte, "Compressor aan/uit", "state", "compressor"},
      {42, FieldType::Byte, "Elektrisch element aan/uit", "state", "e_element"},
      {43, FieldType::Byte, "Trickle heating aan/uit", "state", "trickle"},
      {44, FieldType::Byte, "Fout aanwezig (0=J, 1=N)", "", ""},
      // 31 - 35
      {45, FieldType::Byte, "Vrijkoelen actief (0=uit, 1=aan)", "state", "free_cooling"},
      {48, FieldType::SignedIntDec2, "Kamertemperatuur", "temperature", "room"},
      {50, FieldType::SignedIntDec2, "Kamertemperatuur setpoint", "temperature", "request_room"},
      {52, FieldType::Byte, "Warmtevraag", "request", "heat_thermostat"},
      // 36 - 40
      {53, FieldType::Byte, "State (0=init,1=uit,2=CV,3=boiler,4=vrijkoel,5=ontluchten)", "state",
       "main"},
      {54, FieldType::Byte, "Substatus (255=geen)", "state", "sub"},
      // 41 - 45
      {64, FieldType::Byte, "Fout gevonden (foutcode)", "fault", "code"},
      // 46 - 50
      // 51 - 55
      // 56 - 60
      {89, FieldType::UnsignedInt, "Vrijkoelen interval (sec)", "", ""},
      // 96 - 100
      {142, FieldType::Byte, "Warmtevraag totaal", "request", "heat_total"},
      {143, FieldType::UnsignedInt32, "E-consumption stand-by", "energy", "standby"},
      {147, FieldType::UnsignedInt32, "E-consumption heating", "energy", "heating"},
      // 101 - 105
      {151, FieldType::UnsignedInt32, "E-consumption DHW", "energy", "dhw"},
      {153, FieldType::UnsignedInt32, "E-consumption cooling", "energy", "cooling"},
  };

  Field autotempConfig[kFieldCountAutotemp]{
      {0, FieldType::Byte, "Modus", "", ""},
      {1, FieldType::Byte, "Toestand", "", ""},
      {2, FieldType::Byte, "Status", "", ""},
      {3, FieldType::Byte, "Warmtebron", "", ""},
      {4, FieldType::Byte, "Foutcode", "", ""},
      {5, FieldType::Byte, "Gewenst vermogen", "", ""},
      {6, FieldType::SignedIntDec2, "Ruimte 1 temp", "", ""},
      {8, FieldType::SignedIntDec2, "Ruimte 1 setp", "", ""},
      {10, FieldType::Byte, "Ruimte 1 vermogen %", "", ""},
      {13, FieldType::SignedIntDec2, "Ruimte 2 temp", "", ""},
      {15, FieldType::SignedIntDec2, "Ruimte 2 setp", "", ""},
      {17, FieldType::Byte, "Ruimte 2 vermogen %", "", ""},
      {20, FieldType::SignedIntDec2, "Ruimte 3 temp", "", ""},
      {22, FieldType::SignedIntDec2, "Ruimte 3 setp", "", ""},
      {24, FieldType::Byte, "Ruimte 3 vermogen %", "", ""},
      {27, FieldType::SignedIntDec2, "Ruimte 4 temp", "", ""},
      {29, FieldType::SignedIntDec2, "Ruimte 4 setp", "", ""},
      {31, FieldType::Byte, "Ruimte 4 vermogen %", "", ""},
      {34, FieldType::SignedIntDec2, "Ruimte 5 temp", "", ""},
      {36, FieldType::SignedIntDec2, "Ruimte 5 setp", "", ""},
      {38, FieldType::Byte, "Ruimte 5 vermogen %", "", ""},
      {41, FieldType::SignedIntDec2, "Ruimte 6 temp", "", ""},
      {43, FieldType::SignedIntDec2, "Ruimte 6 setp", "", ""},
      {45, FieldType::Byte, "Ruimte 6 vermogen %", "", ""},
      {90, FieldType::Byte, "Toestand uit", "", ""},
      {91, FieldType::Byte, "Toestand koel", "", ""},
      {92, FieldType::Byte, "Toestand verwarmen", "", ""},
      {93, FieldType::Byte, "Toestand hand", "", ""},
      {142, FieldType::UnsignedInt, "Lege batterij (0=OK)", "", ""},
      {150, FieldType::SignedIntDec2, "Buitentemperatuur", "", ""},
      {152, FieldType::UnsignedInt, "Rest cyclustijd", "", ""},
      {156, FieldType::UnsignedInt, "Comm Ruimte A", "", ""},
      {158, FieldType::UnsignedInt, "Comm Ruimte B", "", ""},
      {160, FieldType::UnsignedInt, "Comm Ruimte C", "", ""},
      {162, FieldType::UnsignedInt, "Comm Ruimte D", "", ""},
      {164, FieldType::UnsignedInt, "Comm Ruimte E", "", ""},
      {166, FieldType::UnsignedInt, "Comm Ruimte F", "", ""}};
};

#endif
