#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <stdio.h>

#define	I2C_ADDRESS	0x41

/*
gcc pimaster2.c -o pimaster2
*/

const char MESSAGE_GETREGELAAR[6] = {0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
const int MESSAGE_GETREGELAAR_LENGTH = 6;
const char MESSAGE_OPHALENSERIENUMMER[6] = {0x80, 0x90, 0xE1, 0x04, 0x00, 0x89};
const int MESSAGE_OPHALENSERIENUMMER_LENGTH = 6;
const char MESSAGE_OPHALENSETTING0[25] = {0x80,0xA4,0x10,0x04,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33};
const int MESSAGE_OPHALENSETTING0_LENGTH = 25;
const char MESSAGE_OPHALENSETTING50[25] = {0x80, 0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x01};
const int MESSAGE_OPHALENSETTING50_LENGTH = 25;
const char MESSAGE_OPHALENSETTING69[25] = {0x80,0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0xEE};
const int MESSAGE_OPHALENSETTING69_LENGTH = 25;
const char MESSAGE_OPHALENDATATYPE[6] = {0x80,0xA4,0x00,0x04,0x00,0x56};
const int MESSAGE_OPHALENDATATYPE_LENGTH = 6;
const char MESSAGE_DATALOG6[6] = {0x80, 0xA4, 0x01, 0x04, 0x00, 0x55};
const int MESSAGE_DATALOG6_LENGTH = 6;
const char MESSAGE_VRAAGCONFIG[10] = {0x80, 0xC0, 0x30, 0x04, 0x04, 0x00, 0x00, 0x00, 0x0C, 0xFA};
const int MESSAGE_VRAAGCONFIG_LENGTH = 10;
const char MESSAGE_VRAAGVENTIELAANWEZIG[10] = {0x80, 0xC0, 0x30, 0x04, 0x04, 0x01, 0x00, 0x00, 0x03, 0x02};
const int MESSAGE_VRAAGVENTIELAANWEZIG_LENGTH = 10;
const char MESSAGE_VRAAGCOUNTERS[6] = {0x80, 0xC2, 0x10, 0x04, 0x00, 0x28};
const int MESSAGE_VRAAGCOUNTERS_LENGTH = 6;


int SendMessage(int fd, const char *message, int length);
int ReadBytes(int fd, int length);
void PrintHelp();

#define KEY_QUESTION 63
#define KEY_R 114
#define KEY_S 115
#define KEY_0 48
#define KEY_5 53
#define KEY_6 54
#define KEY_X 120
#define KEY_D 100	// ophalen datatype
#define KEY_A 97	// ophalen datalog
#define KEY_C 99	// vraag config
#define KEY_I 105	// vraag ventielaanwezig
#define KEY_V 118 	// vraag counters

int main()
{
	int file_i2c;

	//----- OPEN THE I2C BUS -----
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		printf("Failed to open the i2c bus");
		return -1;
	}
	
	int addr = I2C_ADDRESS;          //<<<<<The I2C address of the slave
	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return -1;
	}

	int c;
	do
	{
		printf("Type a command (? = help)\n");
		c = getchar();

		switch (c)
		{
		case KEY_QUESTION:
			PrintHelp();
			break;
			
		case KEY_R:
			if (SendMessage(file_i2c, MESSAGE_GETREGELAAR, MESSAGE_GETREGELAAR_LENGTH)==0)
				printf("Send message GetRegelaar\n");
			break;
		
		case KEY_S:
			if (SendMessage(file_i2c, MESSAGE_OPHALENSERIENUMMER, MESSAGE_OPHALENSERIENUMMER_LENGTH)==0)
				printf("Send message Ophalen Serienummer\n");
			break;
		
		case KEY_0:
			if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING0, MESSAGE_OPHALENSETTING0_LENGTH)==0)
				printf("Send message Ophalen Setting 0");
			break;
			
		case KEY_5:
			if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING50, MESSAGE_OPHALENSETTING50_LENGTH)==0)
				printf("Send message Ophalen Setting 50");
			break;

		case KEY_6:
			if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING69, MESSAGE_OPHALENSETTING69_LENGTH)==0)
				printf("Send message Ophalen Setting 69");
			break;

		case KEY_D:
			if (SendMessage(file_i2c, MESSAGE_OPHALENDATATYPE, MESSAGE_OPHALENDATATYPE_LENGTH)==0)
				printf("Send message ophalen datatype");
			break;

		case KEY_A:	// ophalen datalog
			if (SendMessage(file_i2c, MESSAGE_DATALOG6, MESSAGE_DATALOG6_LENGTH)==0)
				printf("Send message ophalen datalog");
			break;

		case KEY_C:	// vraag config
			if (SendMessage(file_i2c, MESSAGE_VRAAGCONFIG, MESSAGE_VRAAGCONFIG_LENGTH)==0)
				printf("Send message vraag config");
			break;

		case KEY_I:	// vraag ventielaanwezig
			if (SendMessage(file_i2c, MESSAGE_VRAAGVENTIELAANWEZIG, MESSAGE_VRAAGVENTIELAANWEZIG_LENGTH)==0)
				printf("Send message vraag ventiel aanwezig");
			break;
		
		case KEY_V: // vraag counters
			if (SendMessage(file_i2c, MESSAGE_VRAAGCOUNTERS, MESSAGE_VRAAGCOUNTERS_LENGTH)==0)
				printf("Send message vraag counters");
			break;

		case KEY_X:
			printf("Stopping\n");
			break;
		
		default:
			printf("Unknown command\n");
			break;
		}
	} while (c!=KEY_X);
}


void PrintHelp()
{
	printf("? = Help\n");
	printf("r = GetRegelaar\n");
	printf("s = Ophalen Serienummer\n");
	printf("0 = Ophalen Setting 0\n");
	printf("5 = Ophalen Setting 50\n");
	printf("6 = Ophalen Setting 69\n");
	printf("d = Ophalen DataType\n");
	printf("a = Ophalen datalog\n");
	printf("c = Vraag config\n");
	printf("i = Ventiel aanwezig\n");
	printf("v = Vraag counters\n");
	printf("x = Exit\n");
}

int SendMessage(int fd, const char *message, int length)
{
	//----- WRITE BYTES -----
	// The first byte is the destination address, so can be skipped.

	int written = write(fd, message, length);
	if (written != length)		//write() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
	{
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to write message to the i2c bus. Written bytes:%d\n", written);
		return -1;
	}

	return 0;
}


int ReadBytes(int fd, int length)
{
	char buffer[60];

	//----- READ BYTES -----
	if (read(fd, buffer, length) != length)		//read() returns the number of bytes actually read, if it doesn't match then an error occurred (e.g. no response from the device)
	{
		//ERROR HANDLING: i2c transaction failed
		printf("Failed to read from the i2c bus.\n");
		return -1;
	}
	else
	{
		printf("Data read: %s\n", buffer);
		return 0;
	}
}
