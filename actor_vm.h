#ifndef PL_ACTOR_VM_H
#define PL_ACTOR_VM_H

#include "object.h"

typedef struct {
  // Data
  ObjString *message;
  Value *slots;  // function parameters
  int arg_count;
} ActorData;

#endif // PL_ACTOR_VM_H