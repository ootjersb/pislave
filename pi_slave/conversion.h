#ifndef CONVERSION_H_
#define CONVERSION_H_

#include <cstdint>

namespace conversion {

bool IsChecksumValid(unsigned char* buffer, int length);
float ParseSignedIntDec2(unsigned char* buffer);
int ParseByte(unsigned char* buffer);
uint32_t ParseUnsignedInt32(unsigned char* buffer);
unsigned int ParseUnsignedInt(unsigned char* buffer);

} /* namespace conversion */

#endif /* CONVERSION_H_ */
