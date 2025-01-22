#ifndef PL_COMMON_H
#define PL_COMMON_H

#define bool unsigned char
#define true 1
#define false 0

#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

// Only 256, because of small size of chunk
#define UINT8_COUNT (UINT8_MAX + 1)

#endif // PL_COMMON_H