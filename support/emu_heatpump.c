// This program will emulate the heatpump
// Compile: gcc emu_heatpump.c -o emu_heatpump
// run: ./emu_heatpump
#include <fcntl.h>          //Needed for I2C port
#include <linux/i2c-dev.h>  //Needed for I2C port
#include <stdio.h>
#include <sys/ioctl.h>  //Needed for I2C port
#include <unistd.h>     //Needed for I2C port

#define I2C_ADDRESS 0x40  // 0x80 = the SVM USB

const char AUTOTEMP_DATALOG[153] = {
    0x82, 0xA4, 0x01, 0x01, 0x93, 0x08, 0x4D, 0x17, 0xA7, 0x17, 0xEC, 0x07, 0xEE, 0x07, 0x7A, 0x09,
    0x26, 0x08, 0x1F, 0x06, 0x39, 0x05, 0xC9, 0x07, 0xC2, 0x07, 0xCB, 0x00, 0x82, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x08, 0xA0, 0x08, 0x34, 0x00, 0x04, 0xFF, 0x00,
    0x07, 0x8A, 0x08, 0xEC, 0x07, 0x8A, 0x08, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0xAF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x04, 0x00, 0x00, 0x5C, 0xF0, 0xEE, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1C};
const int AUTOTEMP_DATALOG_LENGTH = 153;

int SendMessage(int fd, const char* message, int length);
int ReadBytes(int fd, int length);
void PrintHelp();

#define KEY_QUESTION 63
#define KEY_R 114
#define KEY_S 115
#define KEY_0 48
#define KEY_5 53
#define KEY_6 54
#define KEY_X 120
#define KEY_D 100  // datalog
#define KEY_A 97
#define KEY_C 99
#define KEY_I 105
#define KEY_V 118
#define KEY_O 111

int main() {
  int file_i2c;

  //----- OPEN THE I2C BUS -----
  char* filename = (char*)"/dev/i2c-1";
  if ((file_i2c = open(filename, O_RDWR)) < 0) {
    // ERROR HANDLING: you can check errno to see what went wrong
    printf("Failed to open the i2c bus");
    return -1;
  }

  int addr = I2C_ADDRESS;  //<<<<<The I2C address of the slave
  if (ioctl(file_i2c, I2C_SLAVE, addr) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    // ERROR HANDLING; you can check errno to see what went wrong
    return -1;
  }

  int c;
  do {
    printf("Emulating Heat Pump. Type a command (? = help)\n");
    c = getchar();

    switch (c) {
    case KEY_QUESTION:
      PrintHelp();
      break;

    case KEY_D:
      if (SendMessage(file_i2c, AUTOTEMP_DATALOG, AUTOTEMP_DATALOG_LENGTH) == 0)
        printf("Send message datalog\n");
      break;

    case KEY_X:
      printf("Stopping\n");
      break;

    default:
      printf("Unknown command\n");
      break;
    }
  } while (c != KEY_X);
}

void PrintHelp() {
  printf("? = Help\n");
  printf("d = Versturen datalog\n");
  printf("x = Exit\n");
}

int SendMessage(int fd, const char* message, int length) {
  //----- WRITE BYTES -----
  // The first byte is the destination address, so can be skipped.

  int written = write(fd, message, length);
  if (written != length)  // write() returns the number of bytes actually written, if it doesn't
                          // match then an error occurred (e.g. no response from the device)
  {
    /* ERROR HANDLING: i2c transaction failed */
    printf("Failed to write message to the i2c bus. Written bytes:%d\n", written);
    return -1;
  }

  return 0;
}

int ReadBytes(int fd, int length) {
  char buffer[60];

  //----- READ BYTES -----
  if (read(fd, buffer, length) !=
      length)  // read() returns the number of bytes actually read, if it doesn't match then an
               // error occurred (e.g. no response from the device)
  {
    // ERROR HANDLING: i2c transaction failed
    printf("Failed to read from the i2c bus.\n");
    return -1;
  } else {
    printf("Data read: %s\n", buffer);
    return 0;
  }
}
