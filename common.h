#ifndef PL_COMMON_H
#define PL_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define bool uint8_t
#define true 1
#define false 0

#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

#define DEBUG_STRESS_GC  // If set, start as possible as can
// #define DEBUG_LOG_GC

// Only 2^16, because of small size of chunk
// Need for global vars
#define UINT16_COUNT (UINT16_MAX + 1)

#endif // PL_COMMON_H