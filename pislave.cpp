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
#include "datalogparser.h"
#include "config.h"


using namespace std;

void closeSlave();
int getControlBits(int, bool);
void *work(void *);
void writeToFile(char *filename, string data);
void processIncomingMessage(unsigned char *buffer, int length);
string bufferToReadableString(unsigned char *buffer, int length);
int GetURL(char *myurl);
int insertlog(char *datalog);

const int slaveAddress = 0x40; // <-- 0x40 is 7 bit address of 0x80
bsc_xfer_t xfer; // Struct to control data flow
int command = 0; // -1=exit, 0=read mode, 1=sendgetregelaar
Config config;
DatalogParser p(config.DeviceType);

int main() 
{
    pthread_t cThread;

    gpioInitialise();
    cout << "Initialized GPIOs\n";

	work(NULL);
/*
    cout << "Starting a new thread to receive data" << endl;
    if(pthread_create(&cThread, NULL, work, NULL))
    {
        perror("ERROR creating thread.");
    }

    string strinput;
    string exit_command = "x";

    cout << "Enter a command (x=exit): ";
    while (strinput.compare(exit_command))
    {
        getline(cin, strinput);
        cout << "You entered: " << strinput << endl;
    }
    cout << "stopping" << endl;
    command = -1;
    sleep(1);
    closeSlave();
*/

    return 0;
}

void ConvertCharToHex(unsigned char *buffer, char *converted, int bufferlength)
{
  for(int i=0;i<bufferlength;i++) {
    sprintf(&converted[i*2], "%02X", buffer[i]);
  }
}

void *work(void * parm)
{
    //  TODO: Add the pthread_kill to signal this thread instead of just shutting down

    // Close old device (if any)
    xfer.control = getControlBits(slaveAddress, false); // To avoid conflicts when restarting
    bscXfer(&xfer);
    // Set I2C slave address
    xfer.control = getControlBits(slaveAddress, true);
    int status = bscXfer(&xfer); // Should now be visible in I2C-Scanners

    if (status >= 0)
    {
        cout << "Opened slave\n";
        xfer.rxCnt = 0;
        int loopcount = 0;
        int receivedcount = 0;
        unsigned char *buffer = new unsigned char[1000];
        while(command!=-1)
        {
            bscXfer(&xfer);
            if(xfer.rxCnt > 0) 
            {
                for(int i = 0; i < xfer.rxCnt; i++)
                {
                    buffer[receivedcount] = xfer.rxBuf[i];
                    receivedcount++;
                    // printf("0x%x", );
                    // cout << std::hex << " 0x" << xfer.rxBuf[i];
                }
            }
            else
            {
                loopcount++;
                if (loopcount>10000)
                {
                    if (receivedcount>0)
                    {
                        // write buffer and clear all
						processIncomingMessage(buffer, receivedcount);

                        receivedcount = 0;
                    }
                    loopcount = 0;
                }
                else
                {
                    usleep(10); // 10 us
                }
            }
        }
        cout << "ended loop" << endl;
        delete[] buffer;
    }
    else
	{
        cout << "Failed to open slave!!!\n";
	}
	
	return NULL; // TODO: figure out what should be returned 
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

    return (address << 16 /*= to the start of significant bits*/) | flags;
}

void UploadToDomoticz(char *params)
{
	char domoticzUrl[400];
	sprintf(domoticzUrl, "http://10.0.0.20:8080/json.htm?type=command&%s", params);
	if (config.LogToConsole && config.DebugMode)
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
	if (p.FieldChanged(label))
	{
		UploadSwitchToDomoticz(idx, p.FieldValue(label));
	}
}

void ParseDatalogAutotemp(unsigned char *buffer, int length)
{
	if (!p.ParseWithHeader(buffer, length))
	{
		if (config.LogToConsole)
			printf("Failed to parse header/checksum\n");
		return;
	}

	float outsideTemp = (((int) buffer[155] << 8) + (int) buffer[156])/100.0;

	int commTellerA = ((int) buffer[161]<<8) + (int) buffer[162];
	int commTellerB = ((int) buffer[163]<<8) + (int) buffer[164];
	int commTellerC = ((int) buffer[165]<<8) + (int) buffer[166];
	int commTellerD = ((int) buffer[167]<<8) + (int) buffer[168];
	int commTellerE = ((int) buffer[169]<<8) + (int) buffer[170];
	int commTellerF = ((int) buffer[171]<<8) + (int) buffer[172];
	if (config.LogToConsole)
	{
		printf("Buitentemperatuur: %s\n", p.FieldValue("Buitentemperatuur"));
		printf("Gewenst vermogen: %s\n", p.FieldValue("Gewenst vermogen"));
		printf("Ruimte 1 temp: %s\n", p.FieldValue("Ruimte 1 temp"));
		printf("Ruimte 2 temp: %s\n", p.FieldValue("Ruimte 2 temp"));
		printf("Ruimte 3 temp: %s\n", p.FieldValue("Ruimte 3 temp"));
		printf("Ruimte 4 temp: %s\n", p.FieldValue("Ruimte 4 temp"));
		printf("Ruimte 5 temp: %s\n", p.FieldValue("Ruimte 5 temp"));
		printf("Ruimte 6 temp: %s\n", p.FieldValue("Ruimte 6 temp"));
		printf("Comm tellers A: %d, B: %d, C: %d, D:%d, E: %d, F: %d\n", commTellerA, commTellerB, commTellerC, commTellerD, commTellerE, commTellerF);
		printf("Rest cyclustijd %s\n", p.FieldValue("Rest cyclustijd"));
	}
	
	if (config.LogToDomoticz)
	{
		// Outside temperature is now logged by WPU reader
		// UploadTemperatureToDomoticz(398, outsideTemp);
		UploadTemperatureToDomoticz(592, p.FieldValue("Ruimte 1 temp"));
		UploadTemperatureToDomoticz(576, p.FieldValue("Ruimte 3 temp"));
		UploadTemperatureToDomoticz(577, p.FieldValue("Ruimte 4 temp"));
		UploadTemperatureToDomoticz(578, p.FieldValue("Ruimte 2 temp"));
		UploadTemperatureToDomoticz(579, p.FieldValue("Ruimte 5 temp"));
		UploadTemperatureToDomoticz(580, p.FieldValue("Ruimte 6 temp"));
		UploadTemperatureToDomoticz(581, p.FieldValue("Gewenst vermogen"));
		UploadTemperatureToDomoticz(598, p.FieldValue("Rest cyclustijd"));
		UploadTemperatureToDomoticz(599, p.FieldValue("Comm Ruimte A"));
		UploadTemperatureToDomoticz(595, p.FieldValue("Comm Ruimte B"));
		UploadTemperatureToDomoticz(594, p.FieldValue("Comm Ruimte C"));
		UploadTemperatureToDomoticz(584, p.FieldValue("Comm Ruimte D"));
		UploadTemperatureToDomoticz(596, p.FieldValue("Comm Ruimte E"));
		UploadTemperatureToDomoticz(597, p.FieldValue("Comm Ruimte F"));
		UploadTemperatureToDomoticz(585, p.FieldValue("Ruimte 1 setp"));
		UploadTemperatureToDomoticz(586, p.FieldValue("Ruimte 3 setp"));
		UploadTemperatureToDomoticz(587, p.FieldValue("Ruimte 4 setp"));
		UploadTemperatureToDomoticz(588, p.FieldValue("Ruimte 2 setp"));
		UploadTemperatureToDomoticz(589, p.FieldValue("Ruimte 5 setp"));
		UploadTemperatureToDomoticz(590, p.FieldValue("Ruimte 6 setp"));
		UploadCounterToDomoticz(591, p.FieldValue("Foutcode"));
		UploadTemperatureToDomoticz(593, p.FieldValue("Ruimte 1 vermogen %"));
		UploadTemperatureToDomoticz(600, p.FieldValue("Ruimte 3 vermogen %"));
		UploadTemperatureToDomoticz(601, p.FieldValue("Ruimte 4 vermogen %"));
		UploadTemperatureToDomoticz(602, p.FieldValue("Ruimte 2 vermogen %"));
		UploadTemperatureToDomoticz(603, p.FieldValue("Ruimte 5 vermogen %"));
		UploadTemperatureToDomoticz(604, p.FieldValue("Ruimte 6 vermogen %"));
	}
}


void ParseDatalogHeatPump(unsigned char *buffer, int length)
{
	if (!p.ParseWithHeader(buffer, length))
		return;
	
	if (config.LogToConsole)
		printf("Buitentemperatuur: %s\n", p.FieldValue("Buitentemperatuur"));

	if (config.LogToDomoticz)
	{
		UploadTemperatureToDomoticz(398, p.FieldValue("Buitentemperatuur"));
		UploadTemperatureToDomoticz(436, p.FieldValue("Van bron"));
		UploadTemperatureToDomoticz(437, p.FieldValue("Naar bron"));
		UploadTemperatureToDomoticz(438, p.FieldValue("CV retour"));
		UploadTemperatureToDomoticz(439, p.FieldValue("CV aanvoer"));
		UploadTemperatureToDomoticz(440, p.FieldValue("Flow sensor bron"));
		UploadTemperatureToDomoticz(441, p.FieldValue("Verdamper temperatuur"));
		UploadTemperatureToDomoticz(442, p.FieldValue("Zuiggas temperatuur"));
		UploadTemperatureToDomoticz(443, p.FieldValue("Persgas temperatuur"));
		UploadTemperatureToDomoticz(444, p.FieldValue("Vloeistof temperatuur"));
		UploadTemperatureToDomoticz(446, p.FieldValue("Boiler hoog"));
		UploadTemperatureToDomoticz(447, p.FieldValue("Boiler laag"));
		UploadCounterToDomoticz(448, p.FieldValue("State (0=init,1=uit,2=CV,3=boiler,4=vrijkoel,5=ontluchten)"));
		UploadTemperatureToDomoticz(478, p.FieldValue("Kamertemperatuur"));
		UploadCounterToDomoticz(479, p.FieldValue("Substatus (255=geen)"));
		UploadTemperatureToDomoticz(481, p.FieldValue("Snelheid cv pomp (%)"));
		UploadTemperatureToDomoticz(482, p.FieldValue("Snelheid bron pomp (%)"));
		UploadTemperatureToDomoticz(483, p.FieldValue("Snelheid boiler pomp (%)"));
		CheckChangeUploadSwitch(485, "Compressor aan/uit");
		CheckChangeUploadSwitch(486, "Elektrisch element aan/uit");
		CheckChangeUploadSwitch(487, "Fout aanwezig (0=J, 1=N)");
		UploadCounterToDomoticz(488, p.FieldValue("Fout gevonden (foutcode)"));
		UploadTemperatureToDomoticz(605, p.FieldValue("Warmtevraag"));
		UploadTemperatureToDomoticz(606, p.FieldValue("Vrijkoelen interval (sec)"));
	}
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
	if (config.LogToSqlLite)
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
		sprintf(filename, "/home/pi/pislave/data/%02X%02X-%02X%02X%02X%02X.txt", buffer[1], buffer[2], buffer[5+0], buffer[5+1], buffer[5+2], buffer[5+3]);
	}
	else if (buffer[1] == 0xA4 && buffer[2] == 0x01) // datalog
	{
		messagename = "Datalog";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]); // TODO: add timestamp??
		
		if (config.DeviceType == DEVICE_ID_WARMTEPOMP)
			ParseDatalogHeatPump(buffer, length);
		
		if (config.DeviceType == DEVICE_ID_AUTOTEMP)
			ParseDatalogAutotemp(buffer, length);
	}
	else
	{
		messagename = "Unknown";
		sprintf(filename, "/home/pi/pislave/data/%02X%02X.txt", buffer[1], buffer[2]);
	}
	
	string data = bufferToReadableString(buffer, length);
	if (config.LogToConsole)
	{
		cout << data << endl;
	}
	if (config.LogToFile)
	{
		if (config.LogToConsole)
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
