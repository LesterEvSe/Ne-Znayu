#ifndef PL_COMPILER_H
#define PL_COMPILER_H

#include "vm.h"

bool compile(const char *source, Chunk *chunk);

#endif // PL_COMPILER_H