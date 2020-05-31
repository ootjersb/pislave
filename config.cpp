#include "datalogparser.h"
#include "config.h"

Config::Config()
{
	// DeviceType = DEVICE_ID_WARMTEPOMP;
	DeviceType = DEVICE_ID_AUTOTEMP;
	LogToFile = true;
	LogToSqlLite = false;
	LogToDomoticz = true;
	LogToConsole = false;
	DebugMode = false;
}