#ifndef PL_COMMON_H
#define PL_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define bool uint8_t
#define true 1
#define false 0

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

// Only 2^16, because of small size of chunk
#define UINT16_COUNT (UINT16_MAX + 1)

#endif // PL_COMMON_H