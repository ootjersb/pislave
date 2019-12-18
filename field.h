#ifndef FIELD_H
#define FIELD_H

#include <string>
using namespace std;

enum FieldType { Byte /* 0x00 */, UnsignedInt /* 0x10 of 16*/, SignedIntDec2 /* 0x92 of 146 */ };

struct Field
{
    int offset;
    FieldType fieldType;
    string label;
};
#endif