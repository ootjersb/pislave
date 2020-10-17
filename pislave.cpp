// Description: Tool to receive the data from the Itho device. Currently some hardcoded actions are possible: 
// - Insert raw data into SQLite DB, 
// - Parse a single autotemp value and upload to Domoticz
// - Parse datalog and print to screen
// compile: g++ -l pthread -l pigpio -lcurl -lsqlite3 -o pislave pislave.cpp datalogparser.cpp config.cpp
// run: sudo ./pislave
#include <pthread.h>
#include <pigpio.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <signal.h>

#include "datalogparser.h"
#include "config.h"

#define SIGTERM_MSG "SIGTERM received\n"


using namespace std;

void closeSlave();
int getControlBits(int, bool);
void *registerCallback();
void writeToFile(char *filename, string data);
void processIncomingMessage(unsigned char *buffer, int length);
string bufferToReadableString(unsigned char *buffer, int length);
int GetURL(char *myurl);
int insertlog(char *datalog);
void catch_sigterm();
void sig_term_handler(int signum, siginfo_t *info, void *ptr);

string directory;
const int slaveAddress = 0x40; // <-- 0x40 is 7 bit address of 0x80
bsc_xfer_t xfer; // Struct to control data flow
Config *config;
DatalogParser *p;
int stop = 0;	// 0 = running, 1 = stopping

int main(int argc, char *argv[]) 
{
	directory = argv[0];
	config = new Config(directory);
	p = new DatalogParser(config->DeviceType);
    gpioInitialise();
    catch_sigterm();
    cout << "Initialized GPIOs\n";

    // Close old device (if any)
    xfer.control = getControlBits(slaveAddress, false); // To avoid conflicts when restarting
    bscXfer(&xfer);

	registerCallback();

	if (argc<=1)
	{
		printf("Running in service mode; infinite loop\n");
		while (stop==0)
		{
			sleep(10);
		}
		printf("Stopping service\n");
	}
	else
	{
		if (strcmp("--debug", argv[1])==0)
		{
			printf("Running in debug mode\n");

			string strinput;
			string exit_command = "x";

			cout << "Enter a command (x=exit): ";
			while (strinput.compare(exit_command))
			{
				getline(cin, strinput);
				cout << "You entered: " << strinput << endl;
			}
			cout << "stopping" << endl;
		}
		else
		{
			printf("Unknown option, only --debug is support to run in console mode\n");
		}
	}
    closeSlave();
	delete p;
	delete config;
    return 0;
}

void sig_term_handler(int signum, siginfo_t *info, void *ptr)
{
    write(STDERR_FILENO, SIGTERM_MSG, sizeof(SIGTERM_MSG));
	stop = 1;
}

void catch_sigterm()
{
    static struct sigaction _sigact;

    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction = sig_term_handler;
    _sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGTERM, &_sigact, NULL);
}

void ConvertCharToHex(unsigned char *buffer, char *converted, int bufferlength)
{
  for(int i=0;i<bufferlength;i++) {
    sprintf(&converted[i*2], "%02X", buffer[i]);
  }
}

void simple_bsc_event_callback(int event, uint32_t tick) {
    bsc_xfer_t xfer;
    int i = 0, status;

    xfer.control = getControlBits(slaveAddress, true); // (slaveAddress << 16) | 0x305;
    status = bscXfer(&xfer);
    if (! status ) {
        fprintf(stderr, "process register failed.\n");
    }
    if (xfer.rxCnt){
		unsigned char *buffer = new unsigned char[1000];
		for(int i = 0; i < xfer.rxCnt; i++)
			buffer[i] = xfer.rxBuf[i];

		// write buffer and clear all
		processIncomingMessage(buffer, xfer.rxCnt);
    }

}

void *registerCallback()
{
    int status;
    if ( ! eventSetFunc(PI_EVENT_BSC, simple_bsc_event_callback) ){
        xfer.control = getControlBits(slaveAddress, true); // (slaveAddress << 16) | 0x305;
        status = bscXfer(&xfer);
        if (! status ) {
            fprintf(stderr, "process register failed.\n");
        }
    }
	return NULL; 
}

void closeSlave() {
    xfer.control = getControlBits(slaveAddress, false);
    bscXfer(&xfer);
    cout << "Closed slave.\n";

    gpioTerminate();
    cout << "Terminated GPIOs.\n";
}


int getControlBits(int address /* max 127 */, bool open) {
    /*
    Excerpt from http://abyz.me.uk/rpi/pigpio/cif.html#bscXfer regarding the control bits:

    22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    a  a  a  a  a  a  a  -  -  IT HC TF IR RE TE BK EC ES PL PH I2 SP EN

    Bits 0-13 are copied unchanged to the BSC CR register. See pages 163-165 of the Broadcom 
    peripherals document for full details. 

    aaaaaaa defines the I2C slave address (only relevant in I2C mode)
    IT  invert transmit status flags
    HC  enable host control
    TF  enable test FIFO
    IR  invert receive status flags
    RE  enable receive
    TE  enable transmit
    BK  abort operation and clear FIFOs
    EC  send control register as first I2C byte
    ES  send status register as first I2C byte
    PL  set SPI polarity high
    PH  set SPI phase high
    I2  enable I2C mode
    SP  enable SPI mode
    EN  enable BSC peripheral
    */

    // Flags like this: 0b/*IT:*/0/*HC:*/0/*TF:*/0/*IR:*/0/*RE:*/0/*TE:*/0/*BK:*/0/*EC:*/0/*ES:*/0/*PL:*/0/*PH:*/0/*I2:*/0/*SP:*/0/*EN:*/0;

    int flags;
    if(open)
        flags = /*RE:*/ (1 << 9) | /*TE:*/ (1 << 8) | /*I2:*/ (1 << 2) | /*EN:*/ (1 << 0);
    else // Close/Abort
        flags = /*BK:*/ (1 << 7) | /*I2:*/ (0 << 2) | /*EN:*/ (0 << 0);
	// printf("flags=%d\n", flags);

    return (address << 16 /*= to the start of significant bits*/) | flags;
}

void UploadToDomoticz(char *params)
{
	char domoticzUrl[400];
	sprintf(domoticzUrl, "http://10.0.0.20:8080/json.htm?type=command&%s", params);
	if (config->LogToConsole && config->DebugMode)
		printf("Calling %s\n", domoticzUrl);
	GetURL(domoticzUrl);
}

void UploadCounterToDomoticz(int idx, char *value)
{
	char param[50];
	sprintf(param, "param=udevice&idx=%d&svalue=%s", idx, value);
	UploadToDomoticz(param);
}

// /json.htm?type=command&param=switchlight&idx=99&switchcmd=On
void UploadSwitchToDomoticz(int idx, char *value)
{
	char param[50];
	if (strcmp(value, "1")==0)
		sprintf(param, "param=switchlight&idx=%d&switchcmd=On", idx, value);
	else
		sprintf(param, "param=switchlight&idx=%d&switchcmd=Off", idx, value);
	// printf("%s\n", param);
	UploadToDomoticz(param);
}

void UploadTemperatureToDomoticz(int idx, char *value)
{
	char param[50];
	sprintf(param, "param=udevice&idx=%d&nvalue=0&svalue=%s", idx, value);
	UploadToDomoticz(param);
}


void UploadTemperatureToDomoticz(int idx, float value)
{
	char sValue[50];
	sprintf(sValue, "%.2f", value);
	UploadTemperatureToDomoticz(idx, sValue);
}


void CheckChangeUploadSwitch(int idx, string label)
{
	if (p->FieldChanged(label))
	{
		if (config->LogToConsole)
			printf("%s: %s\n", label.c_str(), p->FieldValue(label));
		
		if (config->LogToDomoticz)
			UploadSwitchToDomoticz(idx, p->FieldValue(label));
	}
}

void LogTemperatureFromLabel(int idx, string label, float max)
{
	if (p->FieldChanged(label))
	{
		float tempValue = atof(p->FieldValue(label));
		if ((max>0.0) && (tempValue>max))
		{
			printf("Skipping bogus temperature %f for label %s\n", tempValue, label.c_str());
			return;
		}
		
		if (config->LogToConsole)
			printf("%s: %s\n", label.c_str(), p->FieldValue(label));
		
		if (config->LogToDomoticz)
			UploadTemperatureToDomoticz(idx, p->FieldValue(label));
	}
}

void LogCounterFromLabel(int idx, string label)
{
	if (p->FieldChanged(label))
	{
		if (config->LogToConsole)
			printf("%s: %s\n", label.c_str(), p->FieldValue(label));
		
		if (config->LogToDomoticz)
			UploadCounterToDomoticz(idx, p->FieldValue(label));
	}
}

void ParseDatalogAutotemp(unsigned char *buffer, int length)
{
	if (!p->ParseWithHeader(buffer, length))
	{
		if (config->LogToConsole)
			printf("Failed to parse header/checksum\n");
		return;
	}

	// Outside temperature is now logged by WPU reader
	// UploadTemperatureToDomoticz(398, outsideTemp);
	LogTemperatureFromLabel(592, "Ruimte 1 temp", 50.0);
	LogTemperatureFromLabel(576, "Ruimte 3 temp", 50.0);
	LogTemperatureFromLabel(577, "Ruimte 4 temp", 50.0);
	LogTemperatureFromLabel(578, "Ruimte 2 temp", 50.0);
	LogTemperatureFromLabel(579, "Ruimte 5 temp", 50.0);
	LogTemperatureFromLabel(580, "Ruimte 6 temp", 50.0);
	LogTemperatureFromLabel(581, "Gewenst vermogen", 100.0);
	LogTemperatureFromLabel(598, "Rest cyclustijd", 2000.0);
	LogTemperatureFromLabel(599, "Comm Ruimte A", 0.0);
	LogTemperatureFromLabel(595, "Comm Ruimte B", 0.0);
	LogTemperatureFromLabel(594, "Comm Ruimte C", 0.0);
	LogTemperatureFromLabel(584, "Comm Ruimte D", 0.0);
	LogTemperatureFromLabel(596, "Comm Ruimte E", 0.0);
	LogTemperatureFromLabel(597, "Comm Ruimte F", 0.0);
	LogTemperatureFromLabel(585, "Ruimte 1 setp", 50.0);
	LogTemperatureFromLabel(586, "Ruimte 3 setp", 50.0);
	LogTemperatureFromLabel(587, "Ruimte 4 setp", 50.0);
	LogTemperatureFromLabel(588, "Ruimte 2 setp", 50.0);
	LogTemperatureFromLabel(589, "Ruimte 5 setp", 50.0);
	LogTemperatureFromLabel(590, "Ruimte 6 setp", 50.0);
	LogTemperatureFromLabel(593, "Ruimte 1 vermogen %", 100.0);
	LogTemperatureFromLabel(600, "Ruimte 3 vermogen %", 100.0);
	LogTemperatureFromLabel(601, "Ruimte 4 vermogen %", 100.0);
	LogTemperatureFromLabel(602, "Ruimte 2 vermogen %", 100.0);
	LogTemperatureFromLabel(603, "Ruimte 5 vermogen %", 100.0);
	LogTemperatureFromLabel(604, "Ruimte 6 vermogen %", 100.0);
	LogCounterFromLabel(591, "Foutcode");
}

void ParseDatalogHeatPump(unsigned char *buffer, int length)
{
	if (!p->ParseWithHeader(buffer, length))
		return;
	
	LogTemperatureFromLabel(398, "Buitentemperatuur", 50.0);
	LogTemperatureFromLabel(436, "Van bron", 50.0);
	LogTemperatureFromLabel(437, "Naar bron", 50.0);
	LogTemperatureFromLabel(438, "CV retour", 50.0);
	LogTemperatureFromLabel(439, "CV aanvoer", 50.0);
	LogTemperatureFromLabel(440, "Flow sensor bron", 2000.0);
	LogTemperatureFromLabel(441, "Verdamper temperatuur", 100.0);
	LogTemperatureFromLabel(442, "Zuiggas temperatuur", 100.0);
	LogTemperatureFromLabel(443, "Persgas temperatuur", 100.0);
	LogTemperatureFromLabel(444, "Vloeistof temperatuur", 100.0);
	LogTemperatureFromLabel(446, "Boiler hoog", 100.0);
	LogTemperatureFromLabel(447, "Boiler laag", 100.0);
	LogTemperatureFromLabel(478, "Kamertemperatuur", 50.0);
	LogTemperatureFromLabel(481, "Snelheid cv pomp (%)", 100.0);
	LogTemperatureFromLabel(482, "Snelheid bron pomp (%)", 100.0);
	LogTemperatureFromLabel(483, "Snelheid boiler pomp (%)", 100.0);
	LogTemperatureFromLabel(605, "Warmtevraag", 100.0);
	LogTemperatureFromLabel(606, "Vrijkoelen interval (sec)", 0.0);
	LogTemperatureFromLabel(615, "Druksensor", 0.0);
	CheckChangeUploadSwitch(485, "Compressor aan/uit");
	CheckChangeUploadSwitch(486, "Elektrisch element aan/uit");
	CheckChangeUploadSwitch(487, "Fout aanwezig (0=J, 1=N)");
	LogCounterFromLabel(448, "State (0=init,1=uit,2=CV,3=boiler,4=vrijkoel,5=ontluchten)");
	LogCounterFromLabel(479, "Substatus (255=geen)");
	LogCounterFromLabel(488, "Fout gevonden (foutcode)");
}

void WriteMessageToSQLLite(unsigned char *buffer, int length)
{
	// Write the message to SQL Lite
	char *bufferhex = new char[2003];    // plus 2 for 80 destination and 1 for \0
	sprintf(bufferhex, "80");
	ConvertCharToHex(buffer, bufferhex + 2, length);
	insertlog(bufferhex);
}

void processIncomingMessage(unsigned char *buffer, int length)
{
	if (config->LogToSqlLite)
		WriteMessageToSQLLite(buffer, length);

	// Then process it further
	char filename[128];
	string messagename;
	if (buffer[1] == 0xA4 && buffer[2] == 0x10)	// ophalen setting
	{
		messagename = "OphalenSetting";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X-%02X.txt", buffer[1], buffer[2], buffer[17+5]);
	}
	else if (buffer[1] == 0xC0 && buffer[2] == 0x30) // ophalen config
	{
		messagename = "OphalenConfig";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X-%02X%02X%02X.txt", buffer[1], buffer[2], buffer[5+0], buffer[5+1], buffer[5+2]);
	}
	else if (buffer[1] == 0xA4 && buffer[2] == 0x01) // datalog
	{
		messagename = "Datalog";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]); // TODO: add timestamp??
		
		if (config->DeviceType == DEVICE_ID_WARMTEPOMP)
			ParseDatalogHeatPump(buffer, length);
		
		if (config->DeviceType == DEVICE_ID_AUTOTEMP)
			ParseDatalogAutotemp(buffer, length);
	}
	else
	{
		messagename = "Unknown";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]);
	}
	
	string data = bufferToReadableString(buffer, length);
	if (config->LogToConsole)
	{
		cout << data << endl;
	}
	if (config->LogToFile)
	{
		if (config->LogToConsole)
		{
			cout << "Writing message " << messagename << " to " << filename << endl;
		}
		writeToFile(filename, data);
	}
}

string bufferToReadableString(unsigned char *buffer, int length)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	ss << "[0x80";
	for (int i = 0; i < length; ++i)
	{
		ss << " 0x" << uppercase << std::setw(2) << static_cast<unsigned>(buffer[i]);
	}
	ss << "]";
	return ss.str();
}

void writeToFile(char *filename, string data)
{
	ofstream myfile;
	myfile.open (filename);
	myfile << data;
	myfile.close();
}

typedef struct string_buffer_s
{
    char * ptr;
    size_t len;
} string_buffer_t;


static void string_buffer_initialize( string_buffer_t * sb )
{
    sb->len = 0;
    sb->ptr = (char *) malloc(sb->len+1);
    sb->ptr[0] = '\0';
}


static void string_buffer_finish( string_buffer_t * sb )
{
    free(sb->ptr);
    sb->len = 0;
    sb->ptr = NULL;
}


static size_t string_buffer_callback( void * buf, size_t size, size_t nmemb, void * data )
{
    string_buffer_t * sb = (string_buffer_t *) data;
    size_t new_len = sb->len + size * nmemb;

    sb->ptr = (char *) realloc( sb->ptr, new_len + 1 );

    memcpy( sb->ptr + sb->len, buf, size * nmemb );

    sb->ptr[ new_len ] = '\0';
    sb->len = new_len;

    return size * nmemb;

}


static size_t header_callback(char * buf, size_t size, size_t nmemb, void * data )
{
    return string_buffer_callback( buf, size, nmemb, data );
}


static size_t write_callback( void * buf, size_t size, size_t nmemb, void * data )
{
    return string_buffer_callback( buf, size, nmemb, data );
}

int GetURL( char * myurl )
{
    CURL * curl;
    CURLcode res;
    string_buffer_t strbuf;

    string_buffer_initialize( &strbuf );

    curl = curl_easy_init();

    if(!curl)
    {
        fprintf(stderr, "Fatal: curl_easy_init() error.\n");
        string_buffer_finish( &strbuf );
        return EXIT_FAILURE;
    }

    curl_easy_setopt(curl, CURLOPT_URL, myurl );
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L );
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback );
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strbuf );
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &strbuf );

    res = curl_easy_perform(curl);

    if( res != CURLE_OK )
    {
        fprintf( stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res) );

        curl_easy_cleanup( curl );
        string_buffer_finish( &strbuf );

        return EXIT_FAILURE;
    }

    // printf( "%s\n\n", strbuf.ptr );

    curl_easy_cleanup( curl );
    string_buffer_finish( &strbuf );

    return EXIT_SUCCESS;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int executesql(char *sql)
{
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  rc = sqlite3_open("sqlite.db", &db);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }
  printf("Executing %s\n", sql);
  rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  return rc;
}

int insertlog(char *datalog)
{
  const char *sqltemplate = "insert into datalog(moment, data) values (current_timestamp, '%s')";
  char *statement = (char *) malloc(strlen(datalog) + strlen(sqltemplate) + 1);
  sprintf(statement, sqltemplate, datalog);
  int rc = executesql(statement);
  free(statement);
  return rc;
}
