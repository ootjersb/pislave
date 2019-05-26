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

int SendMessage(int fd, const char *message, int length);
int ReadBytes(int fd, int length);
void PrintHelp();

#define KEY_QUESTION = 63;
#define KEY_R = 114;
#define KEY_S = 115;
#define KEY_0 = 48;
#define KEY_X = 120;

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

	// show a menu
	/*
	? = 63
	r = 114
	s = 115
	0 = 48
	x = 120
	*/
	printf("Type a command (? = help)")
	int c = getchar();

	switch (c)
	{
	case KEY_QUESTION:
		PrintHelp();
		break;
	
	default:
		break;
	}

	// SendMessage(file_i2c, MESSAGE_GETREGELAAR, MESSAGE_GETREGELAAR_LENGTH);
	// printf("Send message GetRegelaar\n");
	SendMessage(file_i2c, MESSAGE_OPHALENSERIENUMMER, MESSAGE_OPHALENSERIENUMMER_LENGTH);
	printf("Send message GetRegelaar\n");
}


void PrintHelp()
{
	printf("? = Help\n");
	printf("r = GetRegelaar\n");
	printf("s = Ophalen Serienummer\n");
	printf("0 = Ophalen Setting 0\n");
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
