#include "counterparser.h"

#include <cstring>
#include <iostream>

#include "conversion.h"

CounterParser::CounterParser(uint16_t device) {
  if (device == kDeviceIdHeatpump) {
    config = warmtepompConfig;
    fieldCount = kFieldCountHeatpump;
  } else {
    throw "Unknown device type";
  }

  // initialize to empty values
  for (uint16_t x = 0; x < fieldCount; x++) {
    sprintf(fieldValues[x], "%s", "");
    sprintf(previousValues[x], "%s", "");
  }
}

bool CounterParser::ParseWithHeader(unsigned char* buffer, int length) {
  if (!conversion::IsChecksumValid(buffer, length)) {
    std::cout << "Checksum invalid, discarding data" << std::endl;
    return false;
  }
  Parse(buffer + 5, length - 5);
  return true;
}

char* CounterParser::FieldValue(uint16_t index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return fieldValues[index];
}

char* CounterParser::FieldValue(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldValue(x);
}

// parses the data without the header
void CounterParser::Parse(unsigned char* buffer, int /*length*/) {
  // TODO: Nothing is done with the length yet. Should check exceeding the
  // length

  for (uint16_t x = 0; x < fieldCount; x++) {
    std::strcpy(previousValues[x], fieldValues[x]);

    switch (config[x].fieldType) {
    case FieldType::SignedIntDec2: {
      float value = conversion::ParseSignedIntDec2(buffer + config[x].offset);
      // std::cout << config[x].label << "=" << value << std::endl;
      sprintf(fieldValues[x], "%.2f", value);
    } break;

    case FieldType::Byte: {
      int value = conversion::ParseByte(buffer + config[x].offset);
      //      std::cout << config[x].label << "=" << value << std::endl;
      sprintf(fieldValues[x], "%d", value);
    } break;

    case FieldType::UnsignedInt: {
      unsigned int value = conversion::ParseUnsignedInt(buffer + config[x].offset);
      //      std::cout << config[x].label << "=" << value << std::endl;
      sprintf(fieldValues[x], "%d", value);
    } break;

    case FieldType::UnsignedInt32: {
      uint32_t value = conversion::ParseUnsignedInt32(buffer + config[x].offset);
      // std::cout << config[x].label << "=" << value << std::endl;
      sprintf(fieldValues[x], "%d", value);
    } break;
    }
  }
}

int CounterParser::GetFieldIndex(std::string label) {
  uint16_t x;
  for (x = 0; x < fieldCount; x++) {
    if (config[x].label == label)
      break;
  }
  if (x == fieldCount) {
    return -1;
  }
  return x;
}
const std::string CounterParser::FieldCategory(int index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return config[index].category;
}

const std::string CounterParser::FieldCategory(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldCategory(x);
}

const std::string CounterParser::FieldTag(int index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return config[index].tag;
}
const std::string CounterParser::FieldTag(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldTag(x);
}
