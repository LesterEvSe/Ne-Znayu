#ifndef PL_VM_H
#define PL_VM_H

#include "chunk.h"
#include "value.h"

// Can dynamically grow stack, but keep it simple for now
#define STACK_MAX 256

typedef struct {
  Chunk *chunk;

  // ptr that pointing into the middle of the bytecode arr
  // This is Instruction Pointer (x86, x64, etc. call it PC - program counter)
  uint8_t *ip;

  Value stack[STACK_MAX];
  Value *stack_top;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void init_vm();
void free_vm();
InterpretResult interpret(Chunk *chunk);

void push(Value value);
Value pop();
void negate();

#endif // PL_VM_H