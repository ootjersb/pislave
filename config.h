#ifndef CONFIG_H
#define CONFIG_H

class Config
{
	public:
		int DeviceType;
		bool LogToFile;
		bool LogToSqlLite;
		bool LogToDomoticz;
		bool LogToConsole;
		bool DebugMode;
		
		Config();
};

#endif