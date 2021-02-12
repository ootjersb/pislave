#ifndef COUNTERPARSER_H_
#define COUNTERPARSER_H_

#include <cstdint>

#include "field.h"

class CounterParser {
 public:
  CounterParser(uint16_t device);

  bool ParseWithHeader(unsigned char* buffer, int length);
  char* FieldValue(std::string label);
  const std::string FieldCategory(int index);
  const std::string FieldCategory(std::string label);
  const std::string FieldTag(int index);
  const std::string FieldTag(std::string label);

  uint16_t fieldCount;

  static constexpr uint16_t kDeviceIdHeatpump = 13;
  static constexpr uint16_t kDeviceIdAutotemp = 15;

 private:
  void Parse(unsigned char* buffer, int length);
  char* FieldValue(uint16_t index);
  int GetFieldIndex(std::string label);

  static constexpr uint16_t kFieldCountHeatpump = 21;

  char errorMessage[100];

  char fieldValues[kFieldCountHeatpump][50];     // only the bigger of the two taken here
  char previousValues[kFieldCountHeatpump][50];  // only the bigger of the two taken here

  Field* config;

  Field warmtepompConfig[kFieldCountHeatpump]{
      // Offset  - datatype - label - category - tag
      {0, FieldType::Byte, "Items", "", ""},
      // 1 - 5
      {1, FieldType::UnsignedInt, "bedrijf cv pomp", "hours", "ch_pump"},
      {3, FieldType::UnsignedInt, "bedrijf bron pomp", "hours", "source_pump"},
      {5, FieldType::UnsignedInt, "bedrijf boiler pomp", "hours", "dhw_pump"},
      {7, FieldType::UnsignedInt, "bedrijf compressor", "hours", "compressor"},
      {9, FieldType::UnsignedInt, "bedrijf elektrisch element", "hours", "e_element"},
      // 6 - 10
      {11, FieldType::UnsignedInt, "cv bedrijf", "hours", "ch_mode"},
      {13, FieldType::UnsignedInt, "boiler bedrijf", "hours", "dhw_mode"},
      {15, FieldType::UnsignedInt, "vrijkoel bedrijf", "hours", "fc_mode"},
      {17, FieldType::UnsignedInt, "bedrijf", "hours", "system"},
      {19, FieldType::UnsignedInt, "cv pomp starts", "starts", "ch_pump"},
      // 11 - 15
      {21, FieldType::UnsignedInt, "bron pomp starts", "starts", "source_pump"},
      {23, FieldType::UnsignedInt, "boiler pomp starts", "starts", "dhw_pump"},
      {25, FieldType::UnsignedInt, "compressor starts", "starts", "compressor"},
      {27, FieldType::UnsignedInt, "elektrisch element starts", "starts", "e_element"},
      {29, FieldType::UnsignedInt, "cv starts", "starts", "ch_mode"},
      // 16 - 20
      {31, FieldType::UnsignedInt, "boiler starts", "starts", "dhw_mode"},
      {33, FieldType::UnsignedInt, "vrijkoel starts", "starts", "fc_mode"},
      {35, FieldType::UnsignedInt, "systeem starts", "starts", "system"},
      {37, FieldType::UnsignedInt, "bedrijf DHW element", "hours", "dhw_element"},
      {39, FieldType::UnsignedInt, "DHW E-element starts", "starts", "dhw_element"},
  };
};

#endif /* COUNTERPARSER_H_ */
