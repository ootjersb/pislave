#ifndef FIELD_H
#define FIELD_H

#include <cstdint>
#include <string>

enum class FieldType {
  Byte /* 0x00 */,
  UnsignedInt /* 0x10 of 16 */,
  UnsignedInt32 /* 0x20 or 32 */,
  SignedIntDec2 /* 0x92 of 146 */,
};

struct Field {
  uint16_t offset;
  FieldType fieldType;
  std::string label;
  std::string category;
  std::string tag;
};

#endif
