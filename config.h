#pragma once
#include <string>

class Config
{
public:
	int DeviceType;
	bool LogToFile;
	bool LogToSqlLite;
	bool LogToDomoticz;
	bool LogToConsole;
	bool DebugMode;

	Config(std::string directory);
	void SetDefaults();
	void ReadConfigFile();
	void ParseOptionLine(std::string& line, const size_t& pos);
	 
private: 
	std::string configdir;
};


