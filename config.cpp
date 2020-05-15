#include "datalogparser.h"
#include "config.h"

Config::Config()
{
	DeviceType = DEVICE_ID_WARMTEPOMP;
	LogToFile = false;
	LogToSqlLite = false;
	LogToDomoticz = true;
	LogToConsole = false;
	DebugMode = false;
}