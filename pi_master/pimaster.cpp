#include <asm/ioctl.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wiringPiI2C.h>

#include <iostream>

#define I2C_ADDRESS 0x40

// I2C definitions

#define I2C_SLAVE 0x0703
#define I2C_SMBUS 0x0720 /* SMBus-level access */

#define I2C_SMBUS_READ 1
#define I2C_SMBUS_WRITE 0

// SMBus transaction types

#define I2C_SMBUS_QUICK 0
#define I2C_SMBUS_BYTE 1
#define I2C_SMBUS_BYTE_DATA 2
#define I2C_SMBUS_WORD_DATA 3
#define I2C_SMBUS_PROC_CALL 4
#define I2C_SMBUS_BLOCK_DATA 5
#define I2C_SMBUS_I2C_BLOCK_BROKEN 6
#define I2C_SMBUS_BLOCK_PROC_CALL 7 /* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA 8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX 32     /* As specified in SMBus standard */
#define I2C_SMBUS_I2C_BLOCK_MAX 32 /* Not specified but we use same structure */

// Structures used in the ioctl() calls

union i2c_smbus_data {
  uint8_t byte;
  uint16_t word;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 2];  // block [0] is used for length + one more for PEC
};

struct i2c_smbus_ioctl_data {
  char read_write;
  uint8_t command;
  int size;
  union i2c_smbus_data* data;
};

static inline int i2c_smbus_access(int fd, char rw, uint8_t command, int size,
                                   union i2c_smbus_data* data) {
  struct i2c_smbus_ioctl_data args;

  args.read_write = rw;
  args.command = command;
  args.size = size;
  args.data = data;
  return ioctl(fd, I2C_SMBUS, &args);
}

using namespace std;

/*
g++ pimaster.cpp -lwiringPi -o pimaster
*/

// read calibration data with correct LSB/MSB
uint16_t read16(int fd, int reg) {
  return (wiringPiI2CReadReg8(fd, reg + 1) << 8) | wiringPiI2CReadReg8(fd, reg);
}

int setupComms() {
  int fd;

  fd = wiringPiI2CSetup(I2C_ADDRESS);

  return fd;
}

static int GetRegelaar(int fd) {
  /*
          int result = wiringPiI2CWrite (fd, 0x10);
          if(result == -1) return result;
          result = wiringPiI2CWrite (fd, 0x20);
          if(result == -1) return result;
          result = wiringPiI2CWrite (fd, 0x30);
          if(result == -1) return result;
  */
  // union i2c_smbus_data data
  // data.byte = 0x10;
  int data = 0x10;
  int result = i2c_smbus_access(fd, I2C_SMBUS_WRITE, data, I2C_SMBUS_BYTE, NULL);

  return result;
}

int main() {
  int fd, result;
  fd = setupComms();
  if (fd < 0) {
    cout << "Failed to setup device" << endl;
    return -1;
  }

  result = GetRegelaar(fd);
  cout << "GetRegelaar: 0x" << hex << result << endl;
}
