#include "config.h"
#include <fstream>
#include <sstream>
#include <string>

Config::Config(std::string directory)
{
	configdir = directory;
	SetDefaults();
	ReadConfigFile();
}

void Config::SetDefaults()
{
	DeviceType = 15;
	LogToFile = true;
	LogToSqlLite = false;
	LogToDomoticz = true;
	LogToConsole = false;
	DebugMode = false;
}

void Config::ReadConfigFile()
{
	size_t slash_pos = configdir.rfind("/");
	std::string configfile = configdir.substr(0, slash_pos)+"/config.txt";
	printf("Reading configuration file %s\n", configfile.c_str());
	std::ifstream infile(configfile);
	// std::ifstream infile("/home/pi/pislave/config.txt");
	std::string line;
	while (std::getline(infile, line))
	{
		if (line.size() > 1 && line.substr(0, 1) == "#") // comment line
			continue;

		size_t pos = line.find('=', 0);
		if (pos > 0 && pos != 4294967295)
		{
			ParseOptionLine(line, pos);
		}
	}
}

void Config::ParseOptionLine(std::string& line, const size_t& pos)
{
	std::string option = line.substr(0, pos);
	std::string value = line.substr(pos + 1, line.size() - pos - 1);

	if (option == "DeviceType")
	{
		DeviceType = std::stoi(value);
		printf("DeviceType=%s\n", value.c_str()); 
	}
	else if (option == "LogToFile")
		LogToFile = value == "true";
	else if (option == "LogToSqlLite")
		LogToSqlLite = value == "true";
	else if (option == "LogToDomoticz")
		LogToDomoticz = value == "true";
	else if (option == "LogToConsole")
		LogToConsole = value == "true";
	else if (option == "DebugMode")
		DebugMode = value == "true";
}