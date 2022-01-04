// gcc pimaster2.c -o pimaster2
#include <fcntl.h>          // Needed for I2C port
#include <linux/i2c-dev.h>  // Needed for I2C port
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  // Needed for I2C port
#include <unistd.h>     // Needed for I2C port

#define I2C_ADDRESS_82 0x41  // 0x82 with 7 most significant bits
#define I2C_ADDRESS_60 0x30  // 0x60 with 7 most significant bits

const char MESSAGE_GETREGELAAR82[6] = {0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
const int MESSAGE_GETREGELAAR82_LENGTH = 6;
const char MESSAGE_OPHALENSERIENUMMER[6] = {0x80, 0x90, 0xE1, 0x04, 0x00, 0x89};
const int MESSAGE_OPHALENSERIENUMMER_LENGTH = 6;
const char MESSAGE_OPHALENSETTING0[25] = {0x80, 0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33};
const int MESSAGE_OPHALENSETTING0_LENGTH = 25;
const char MESSAGE_OPHALENSETTING50[25] = {0x80, 0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x01};
const int MESSAGE_OPHALENSETTING50_LENGTH = 25;
const char MESSAGE_OPHALENSETTING69[25] = {0x80, 0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0xEE};
const int MESSAGE_OPHALENSETTING69_LENGTH = 25;
const char MESSAGE_OPHALENDATATYPE[6] = {0x80, 0xA4, 0x00, 0x04, 0x00, 0x56};
const int MESSAGE_OPHALENDATATYPE_LENGTH = 6;
const char MESSAGE_DATALOG6[6] = {0x80, 0xA4, 0x01, 0x04, 0x00, 0x55};
const int MESSAGE_DATALOG6_LENGTH = 6;
const char MESSAGE_VRAAGCONFIG[10] = {0x80, 0xC0, 0x30, 0x04, 0x04, 0x00, 0x00, 0x00, 0x0C, 0xFA};
const int MESSAGE_VRAAGCONFIG_LENGTH = 10;
const char MESSAGE_VRAAGVENTIELAANWEZIG[10] = {0x80, 0xC0, 0x30, 0x04, 0x04,
                                               0x01, 0x00, 0x00, 0x03, 0x02};
const int MESSAGE_VRAAGVENTIELAANWEZIG_LENGTH = 10;
const char MESSAGE_VRAAGCOUNTERS[6] = {0x80, 0xC2, 0x10, 0x04, 0x00, 0x28};
const int MESSAGE_VRAAGCOUNTERS_LENGTH = 6;
const int MESSAGE_SENDSETTING_LENGTH = 25;
const char MESSAGE_SENDSETTING[25] = {0x80, 0xA4, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x01,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0xA9};
const char MESSAGE_C220[6] = {0x80, 0xC2, 0x20, 0x04, 0x00, 0x18};
const int MESSAGE_C220_LENGTH = 6;
char ophalenSettingMessage[26] = {0x82, 0x80, 0xA4, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33};
char ophalenConfigMessage[11] = {0x82, 0x80, 0xC0, 0x30, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0x04};

int SendMessage(int fd, const char* message, int length);
int ReadBytes(int fd, int length);
void PrintHelp();
char* ConstructOphalensetting(int settingNr);
char CalculateChecksum(int length, char* buffer);
char* ConstructOphalenConfig(int settingNr, int counterNr);
int OpenBus(int addr);
void SendMessageForInput(int file_i2c, int c);
void HandleKeyBoardInput(int file_i2c);
int ReadInputFile(int file_i2c, char* filename);
void ConvertHexToChar(char* hex, char* buffer);

#define KEY_QUESTION 63
#define KEY_R 114
#define KEY_S 115
#define KEY_0 48
#define KEY_2 50
#define KEY_5 53
#define KEY_6 54
#define KEY_X 120
#define KEY_D 100    // ophalen datatype
#define KEY_UP_O 79  // ophalen autotemp settings
#define KEY_A 97     // ophalen datalog
#define KEY_C 99     // vraag config
#define KEY_I 105    // vraag ventielaanwezig
#define KEY_V 118    // vraag counters
#define KEY_O 111    // ophalen alle settings
#define KEY_N 110    // ophalen alle configs
#define KEY_Z 122    // zenden setting 135

int main(int argc, char* argv[]) {
  int file_i2c_82 = OpenBus(I2C_ADDRESS_82);
  if (file_i2c_82 < 0)
    return file_i2c_82;

  if (argc < 2)  // No arguments means keyboard input
  {
    HandleKeyBoardInput(file_i2c_82);
  } else if (argc ==
             2)  // One argument, means the keyboard input has been placed on the command line
  {
    int c = atoi(argv[1]);
    SendMessageForInput(file_i2c_82, c);
    // c = getchar();
  } else if (argc == 3) {
    // This could be -f input.txt
    // Read the input message from the given input file, and then send it
    if (strcmp(argv[1], "-f") != 0) {
      printf("option is not supported; only -f supported\n");
      return 1;
    }
    return ReadInputFile(file_i2c_82, argv[2]);
  }
  return 0;
}

int ReadInputFile(int file_i2c, char* filename) {
  int ch;
  FILE* fp;
  char byteBuffer[2];
  char output[1024];

  printf("Reading input file %s\n", filename);
  fp = fopen(filename, "r");  // read mode.
  if (fp == NULL) {
    printf("failed to open file\n");
    return 2;
  }
  int count = 0;
  int offset = 0;
  do {
    ch = fgetc(fp);
    if ((ch == '[') || (ch == ']') || (ch == ' ')) {
      continue;
    }
    if (ch == EOF) {
      break;
    }

    byteBuffer[count] = ch;
    count++;
    if (count == 2) {
      count = 0;
      // check contents; if 0x then ignore
      if ((byteBuffer[0] == '0') && (byteBuffer[1] == 'x')) {
        continue;
      }

      output[offset++] = byteBuffer[0];
      output[offset++] = byteBuffer[1];
    }
  } while (1);

  for (int i = 0; i < offset; i++) {
    printf("%c", output[i]);
  }
  printf("\n");

  char buffer[offset];
  ConvertHexToChar(output, buffer);
  int startpos = 0;
  if (buffer[0] == 0x82) {
    startpos = 1;
  }
  int length = offset / 2;
  if (SendMessage(file_i2c, buffer + startpos, length - startpos) == 0) {
    printf("Send message, %d bytes\n", length - startpos);
  }

  fclose(fp);
  return 0;
}

void ConvertHexToChar(char* hex, char* buffer) {
  char* pos = hex;
  int length = strlen(hex) / 2;

  /* WARNING: no sanitization or error-checking whatsoever */
  for (size_t count = 0; count < length; count++) {
    sscanf(pos, "%2hhx", &buffer[count]);
    pos += 2;
  }
}

void HandleKeyBoardInput(int file_i2c) {
  int c;
  do {
    if (c != 10) {
      printf("Type a command (? = help)\n");
    }
    c = getchar();
    SendMessageForInput(file_i2c, c);
  } while (c != KEY_X);
}

void SendMessageForInput(int file_i2c, int c) {
  switch (c) {
  case KEY_QUESTION:
    PrintHelp();
    break;

  case KEY_R:
    if (SendMessage(file_i2c, MESSAGE_GETREGELAAR82, MESSAGE_GETREGELAAR82_LENGTH) == 0)
      printf("Send message GetRegelaar\n");
    break;

  case KEY_S:
    if (SendMessage(file_i2c, MESSAGE_OPHALENSERIENUMMER, MESSAGE_OPHALENSERIENUMMER_LENGTH) == 0)
      printf("Send message Ophalen Serienummer\n");
    break;

  case KEY_0:
    if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING0, MESSAGE_OPHALENSETTING0_LENGTH) == 0)
      printf("Send message Ophalen Setting 0\n");
    break;

  case KEY_5:
    if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING50, MESSAGE_OPHALENSETTING50_LENGTH) == 0)
      printf("Send message Ophalen Setting 50\n");
    break;

  case KEY_2:
    ConstructOphalensetting(62);
    if (SendMessage(file_i2c, ophalenSettingMessage + 1, 25) == 0)
      printf("Send message Ophalen Setting 62\n");
    break;

  case KEY_6:
    if (SendMessage(file_i2c, MESSAGE_OPHALENSETTING69, MESSAGE_OPHALENSETTING69_LENGTH) == 0)
      printf("Send message Ophalen Setting 69\n");
    break;

  case KEY_D:
    if (SendMessage(file_i2c, MESSAGE_OPHALENDATATYPE, MESSAGE_OPHALENDATATYPE_LENGTH) == 0)
      printf("Send message ophalen datatype\n");
    break;

  case KEY_A:  // ophalen datalog
    if (SendMessage(file_i2c, MESSAGE_DATALOG6, MESSAGE_DATALOG6_LENGTH) == 0)
      printf("Send message ophalen datalog\n");
    break;

  case KEY_C:  // vraag config
    if (SendMessage(file_i2c, MESSAGE_VRAAGCONFIG, MESSAGE_VRAAGCONFIG_LENGTH) == 0)
      printf("Send message vraag config\n");
    break;

  case KEY_I:  // vraag ventielaanwezig
    if (SendMessage(file_i2c, MESSAGE_VRAAGVENTIELAANWEZIG, MESSAGE_VRAAGVENTIELAANWEZIG_LENGTH) ==
        0)
      printf("Send message vraag ventiel aanwezig\n");
    break;

  case KEY_V:  // vraag counters
    if (SendMessage(file_i2c, MESSAGE_VRAAGCOUNTERS, MESSAGE_VRAAGCOUNTERS_LENGTH) == 0)
      printf("Send message vraag counters\n");
    break;

  case KEY_O:
    for (int i = 0; i <= 0x94; i++) {
      ConstructOphalensetting(i);
      if (SendMessage(file_i2c, ophalenSettingMessage + 1, 25) == 0)
        printf("Send message Ophalensetting(%d)\n", i);
      else
        break;
      sleep(5);  // 1 second
    }
    break;

  case KEY_UP_O:
    for (int i = 0; i <= 95; i++) {
      ConstructOphalensetting(i);
      if (SendMessage(file_i2c, ophalenSettingMessage + 1, 25) == 0)
        printf("Send message Ophalensetting(%d)\n", i);
      else
        break;
      sleep(5);  // 1 second
    }
    break;

  case KEY_N:
    for (int i = 0; i <= 0x26; i++) {
      ConstructOphalenConfig(1, i);
      if (SendMessage(file_i2c, ophalenConfigMessage + 1, 10) == 0)
        printf("Send message OphalenConfig(1, %d)\n", i);
      else
        break;
      sleep(5);  // 1 second
    }
    for (int i = 0; i <= 0x29; i++) {
      ConstructOphalenConfig(0, i);
      if (SendMessage(file_i2c, ophalenConfigMessage + 1, 10) == 0)
        printf("Send message OphalenConfig(0, %d)\n", i);
      else
        break;
      sleep(5);  // 1 second
    }
    break;

  case KEY_Z:  // zenden setting
    if (SendMessage(file_i2c, MESSAGE_SENDSETTING, MESSAGE_SENDSETTING_LENGTH) == 0)
      printf("Send message to set setting 134\n");
    break;

  case (int)'q':
    if (SendMessage(file_i2c, MESSAGE_C220, MESSAGE_C220_LENGTH) == 0)
      printf("Send message C2 20\n");
    break;

  case KEY_X:
    printf("Stopping\n");
    break;

  case 10:  // ignore the return
    break;

  default:
    printf("Unknown command (%d)\n", c);
    break;
  }
}

int OpenBus(int addr) {
  //----- OPEN THE I2C BUS for specified slave -----
  int file_i2c;
  char* filename = (char*)"/dev/i2c-1";
  if ((file_i2c = open(filename, O_RDWR)) < 0) {
    // ERROR HANDLING: you can check errno to see what went wrong
    printf("Failed to open the i2c bus");
    return -1;
  }

  if (ioctl(file_i2c, I2C_SLAVE, addr) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    // ERROR HANDLING; you can check errno to see what went wrong
    return -1;
  }
  return file_i2c;
}

void PrintHelp() {
  printf("? = Help\n");
  printf("q = Versturen C2 20\n");
  printf("r = GetRegelaar\n");
  printf("s = Ophalen Serienummer\n");
  printf("o = Ophalen alle WPU settings (0x00 tot 0x94 - 148)\n");
  printf("O = Ophalen alle Autotemp settings (0 t/m 95)\n");
  printf("0 = Ophalen Setting 0\n");
  printf("2 = Ophalen Setting 62\n");
  printf("5 = Ophalen Setting 50\n");
  printf("6 = Ophalen Setting 69\n");
  printf("d = Ophalen DataType\n");
  printf("a = Ophalen datalog\n");
  printf("c = Vraag config\n");
  printf("n = Vraag alle configs (warmtepomp)\n");
  printf("i = Ventiel aanwezig\n");
  printf("v = Vraag counters\n");
  printf("z = Zending setting 135 - 0.1\n");
  printf("x = Exit\n");
}

int SendMessage(int fd, const char* message, int length) {
  //----- WRITE BYTES -----
  // The first byte is the destination address, so can be skipped.

  int written = write(fd, message, length);
  /*
   * //write() returns the number of bytes actually written, if it doesn't match then an error
   *    occurred (e.g. no response from the device)
   */
  if (written != length) {
    /* ERROR HANDLING: i2c transaction failed */
    printf("Failed to write message to the i2c bus. Written bytes:%d\n", written);
    return -1;
  }

  return 0;
}

int ReadBytes(int fd, int length) {
  char buffer[60];

  //----- READ BYTES -----
  /*
   * read() returns the number of bytes actually read, if it doesn't match then an error
   *    occurred(e.g. no response from the device)
   */
  if (read(fd, buffer, length) != length) {
    // ERROR HANDLING: i2c transaction failed
    printf("Failed to read from the i2c bus.\n");
    return -1;
  } else {
    printf("Data read: %s\n", buffer);
    return 0;
  }
}

char* ConstructOphalenConfig(int settingNr, int counterNr) {
  // data byte 0 is called settingNr
  // data byte 2 is called counterNr
  ophalenConfigMessage[6 + 0] = (char)settingNr;
  ophalenConfigMessage[6 + 2] = (char)counterNr;
  ophalenConfigMessage[10] = CalculateChecksum(10, ophalenConfigMessage);
}

char* ConstructOphalensetting(int settingNr) {
  ophalenSettingMessage[17 + 6] = (char)settingNr;
  ophalenSettingMessage[25] = CalculateChecksum(25, ophalenSettingMessage);
}

char CalculateChecksum(int length, char* buffer) {
  int total = 0;

  for (int i = 0; i < length; i++) {
    total += (int)buffer[i];
  }

  int checksum = 256 - (total % 256);

  if (checksum == 256) {
    checksum = 0;
  }
  return (char)checksum;
}
