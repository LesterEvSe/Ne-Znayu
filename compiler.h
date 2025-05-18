#ifndef PL_COMPILER_H
#define PL_COMPILER_H

#include "object.h"

ObjFunction *compile(const char *source);
void mark_compiler_roots();

#endif // PL_COMPILER_H