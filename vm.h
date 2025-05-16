#ifndef PL_VM_H
#define PL_VM_H

#include "object.h"
#include "table.h"
#include "value.h"
#include "global_vars.h"

#define FRAMES_MAX 256

typedef struct {
  ObjClosure *closure;

  // ptr that pointing into the middle of the bytecode arr
  // This is Instruction Pointer (x86, x64, etc. call it PC - program counter)
  uint16_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;

  int capacity;
  Value *stack;

  Value *stack_top;
  GlobalVarArray globals;

  // For String Interning
  // Make sure that strings with the same chars have the same memory
  Table strings;
  
  ObjString *init_string;  // init keyword for actors
  ObjUpvalue *open_upvalues;

  size_t bytes_allocated;
  size_t next_gc; // some threshold
  
  Obj *objects;  // head of the list
  int gray_count;
  int gray_capacity;
  Obj **gray_stack;
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