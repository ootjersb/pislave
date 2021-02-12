#include "conversion.h"

namespace conversion {

bool IsChecksumValid(unsigned char* buffer, int length) {
  /*
   c# code:
   int runningsum = 0;
   int index = 0;
   while (index < data.Length)
   {
   runningsum += data[index];
   index++;
   }
   int checksum = 256 - (runningsum % 256);
   if (checksum == 256)
   return 0;
   return (byte) checksum;
   */
  // printf("Calculating checksum for %d bytes \n", length);
  int runningsum = 0x80;  // the destination was not included in the header. hard coded here.
  int index = 0;
  while (index < (length - 1)) {
    runningsum += buffer[index];
    // printf("runningsum+=%d\n", buffer[index]);
    index++;
  }
  int checksum = 256 - (runningsum % 256);
  if (checksum == 256) {
    return (checksum == 0);
  }

  // printf("Checksum calculated %d, checksum in data %d\n", checksum,
  // buffer[length-1]);

  return (checksum == buffer[length - 1]);
}

float ParseSignedIntDec2(unsigned char* buffer) {
  int num = ((int)buffer[0] << 8) + buffer[1];
  if (num >= 32768) {
    num -= 65536;
  }
  return num / 100.0;
}

int ParseByte(unsigned char* buffer) { return buffer[0]; }

uint32_t ParseUnsignedInt32(unsigned char* buffer) {
  uint32_t retval = 0;

  retval = static_cast<uint32_t>(buffer[0]) << 24 | static_cast<uint32_t>(buffer[1]) << 16 |
           static_cast<uint32_t>(buffer[2]) << 8 | buffer[3];

  return retval;
}

unsigned int ParseUnsignedInt(unsigned char* buffer) {
  return ((unsigned int)buffer[0] << 8) + buffer[1];
}

} /* namespace conversion */
