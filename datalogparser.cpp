#include "field.h"
#include "datalogparser.h"
#include <iostream>
#include <string.h>

DatalogParser::DatalogParser(int device)
{
    if (device == 13)
    {
        config = warmtepompConfig;
        fieldCount = FIELD_COUNT_WARMTEPOMP;
    }
    else if (device == 15)
    {
        config = autotempConfig;
        fieldCount = FIELD_COUNT_AUTOTEMP;
    }
    else
    {
        throw "Unknown device type";
    }
    
}

float DatalogParser::ParseSignedIntDec2(unsigned char *buffer)
{
    int num = ((int) buffer[0] << 8) + buffer[1];
    return num / 100.0;
}

int DatalogParser::ParseByte(unsigned char *buffer)
{
    return buffer[0];
}

unsigned int DatalogParser::ParseUnsignedInt(unsigned char *buffer)
{
    return ((unsigned int) buffer[0] << 8) + buffer[1];
}

void DatalogParser::Parse(unsigned char *buffer, int length) 
{
    for (int x=0; x<fieldCount;x++)
    {
        if (config[x].fieldType == SignedIntDec2)
        {
            float value = ParseSignedIntDec2(buffer + config[x].offset);
            std::cout << config[x].label << "=" << value << std::endl;
        }
        else if (config[x].fieldType == Byte)
        {
            int value = ParseByte(buffer + config[x].offset);
            std::cout << config[x].label << "=" << value << std::endl;
        }
        else if (config[x].fieldType == UnsignedInt)
        {
            unsigned int value = ParseUnsignedInt(buffer + config[x].offset);
            std::cout << config[x].label << "=" << value << std::endl;
        }
    }
}

int DatalogParser::char2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::invalid_argument("Invalid input string");
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
void DatalogParser::hex2bin(const char* src, unsigned char* target)
{
  while(*src && src[1])
  {
    *(target++) = char2int(*src)*16 + char2int(src[1]);
    src += 2;
  }
}

void DatalogParser::Parse(char *hexstring)
{
    int length = strlen(hexstring) /2;
    unsigned char *buffer = new unsigned char [length];
    hex2bin(hexstring, buffer);
    Parse(buffer + 6, length - 6);
    delete[] buffer;
}
