#ifndef PL_COMPILER_H
#define PL_COMPILER_H

#include "object.h"
#include "vm.h"

ObjFunction *compile(const char *source);

#endif // PL_COMPILER_H