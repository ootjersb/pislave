#include "field.h"
#include "datalogparser.h"
#include <iostream>
#include <string.h>

DatalogParser::DatalogParser(int device)
{
    if (device == DEVICE_ID_WARMTEPOMP)
    {
        config = warmtepompConfig;
        fieldCount = FIELD_COUNT_WARMTEPOMP;
    }
    else if (device == DEVICE_ID_AUTOTEMP)
    {
        config = autotempConfig;
        fieldCount = FIELD_COUNT_AUTOTEMP;
    }
    else
    {
        throw "Unknown device type";
    }

	// initialize to empty values
    for (int x=0; x<fieldCount;x++)
	{
		sprintf(fieldValues[x], "");
		sprintf(previousValues[x], "");
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
	// TODO: Nothing is done with the length yet. Should check exceeding the length
	
    for (int x=0; x<fieldCount;x++)
    {
		strcpy(previousValues[x], fieldValues[x]);
        if (config[x].fieldType == SignedIntDec2)
        {
            float value = ParseSignedIntDec2(buffer + config[x].offset);
            //std::cout << config[x].label << "=" << value << std::endl;
			sprintf(fieldValues[x], "%.2f", value);
        }
        else if (config[x].fieldType == Byte)
        {
            int value = ParseByte(buffer + config[x].offset);
            //std::cout << config[x].label << "=" << value << std::endl;
			sprintf(fieldValues[x], "%d", value);
        }
        else if (config[x].fieldType == UnsignedInt)
        {
            unsigned int value = ParseUnsignedInt(buffer + config[x].offset);
            //std::cout << config[x].label << "=" << value << std::endl;
			sprintf(fieldValues[x], "%d", value);
        }
    }
}


char *DatalogParser::FieldValue(int index)
{
	
	if (index>fieldCount)
	{
		sprintf(errorMessage, "Index too large");
		return errorMessage;
	}
	
	return fieldValues[index];
}


int DatalogParser::GetFieldIndex(string label)
{
	int x;
	for (x=0; x<fieldCount; x++)
	{
		if (config[x].label == label)
			break;
	}
	if (x==fieldCount)
	{
		return -1;
	}
	return x;
}


char *DatalogParser::FieldValue(string label)
{
	int x = GetFieldIndex(label);
	if (x==-1)
	{
		sprintf(errorMessage, "Field not found");
		return errorMessage;
	}

	return FieldValue(x);
}


bool DatalogParser::FieldChanged(int index)
{
	// printf("Comparing '%s' to '%s'\n", fieldValues[index], previousValues[index]);
	return strcmp(fieldValues[index], previousValues[index])!=0;
}


bool DatalogParser::FieldChanged(string label)
{
	int x = GetFieldIndex(label);
	// printf("Field index %d\n", x);
	if (x==-1)
		return false;
	return FieldChanged(x);
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
