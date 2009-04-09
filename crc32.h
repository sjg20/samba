#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
unsigned int crc32(uint32_t crc, const char *buf, unsigned int len);

#endif

