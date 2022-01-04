#include "datalogparser.h"

#include <cstring>
#include <iostream>

#include "conversion.h"
#include "field.h"

DatalogParser::DatalogParser(uint16_t device) {
  if (device == kDeviceIdHeatpump) {
    config = warmtepompConfig;
    fieldCount = kFieldCountHeatpump;
  } else if (device == kDeviceIdAutotemp) {
    config = autotempConfig;
    fieldCount = kFieldCountAutotemp;
  } else {
    throw "Unknown device type";
  }

  // initialize to empty values
  for (uint16_t x = 0; x < fieldCount; x++) {
    sprintf(fieldValues[x], "%s", "");
    sprintf(previousValues[x], "%s", "");
  }
}

bool DatalogParser::ParseWithHeader(unsigned char* buffer, int length) {
  if (!conversion::IsChecksumValid(buffer, length)) {
    std::cout << "Checksum invalid, discarding data" << std::endl;
    return false;
  }
  Parse(buffer + 5, length - 5);
  return true;
}

int DatalogParser::GetFieldIndex(std::string label) {
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

char* DatalogParser::FieldValue(uint16_t index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return fieldValues[index];
}

char* DatalogParser::FieldValue(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldValue(x);
}

bool DatalogParser::FieldChanged(uint16_t index) {
  // printf("Comparing '%s' to '%s'\n", fieldValues[index],
  // previousValues[index]);
  return strcmp(fieldValues[index], previousValues[index]) != 0;
}

bool DatalogParser::FieldChanged(std::string label) {
  int x = GetFieldIndex(label);
  // printf("Field index %d\n", x);
  if (x == -1)
    return false;
  return FieldChanged(x);
}

const std::string DatalogParser::FieldCategory(int index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return config[index].category;
}
const std::string DatalogParser::FieldCategory(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldCategory(x);
}

const std::string DatalogParser::FieldTag(int index) {
  if (index > fieldCount) {
    sprintf(errorMessage, "Index too large");
    return errorMessage;
  }

  return config[index].tag;
}
const std::string DatalogParser::FieldTag(std::string label) {
  int x = GetFieldIndex(label);
  if (x == -1) {
    sprintf(errorMessage, "Field not found");
    return errorMessage;
  }

  return FieldTag(x);
}

int DatalogParser::char2int(char input) {
  if (input >= '0' && input <= '9')
    return input - '0';
  if (input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if (input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::invalid_argument("Invalid input string");
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
void DatalogParser::hex2bin(const char* src, unsigned char* target) {
  while (*src && src[1]) {
    *(target++) = char2int(*src) * 16 + char2int(src[1]);
    src += 2;
  }
}

void DatalogParser::Parse(char* hexstring) {
  uint16_t length = std::strlen(hexstring) / 2;
  unsigned char* buffer = new unsigned char[length];
  hex2bin(hexstring, buffer);
  Parse(buffer + 6, length - 6);
  delete[] buffer;
}

// parses the data without the header
void DatalogParser::Parse(unsigned char* buffer, int /*length*/) {
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
      // std::cout << config[x].label << "=" << value << std::endl;
      sprintf(fieldValues[x], "%d", value);
    } break;

    case FieldType::UnsignedInt: {
      unsigned int value = conversion::ParseUnsignedInt(buffer + config[x].offset);
      // std::cout << config[x].label << "=" << value << std::endl;
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
