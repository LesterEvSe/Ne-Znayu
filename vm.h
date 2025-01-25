#ifndef PL_VM_H
#define PL_VM_H

#include "chunk.h"
#include "table.h"
#include "value.h"
#include "global_vars.h"

typedef struct {
  Chunk *chunk;

  // ptr that pointing into the middle of the bytecode arr
  // This is Instruction Pointer (x86, x64, etc. call it PC - program counter)
  uint16_t *ip;

  int capacity;
  Value *stack;

  Value *stack_top;
  GlobalVarArray globals;

  // For String Interning
  // Make sure that strings with the same chars have the same memory
  Table strings;
  Obj *objects;  // head of the list
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void init_vm();
void free_vm();
InterpretResult interpret(const char *source);

void push(Value value);
Value pop();
void negate();

#endif // PL_VM_H