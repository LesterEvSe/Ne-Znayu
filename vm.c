#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

// Can be recoded with pointer variable, which pass from main func
VM vm;

static void reset_stack() {
  vm.stack_top = vm.stack;
}

void init_vm() {
  reset_stack();
}

void free_vm() {
}

void push(Value value) {
  *vm.stack_top++ = value;
}

Value pop() {
  return *--vm.stack_top;
}

void negate() {
  Value *temp = vm.stack_top-1;
  *temp = -*temp;
}

// About 90% of time PL was inside that function.
// Because it's heart of the VM
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
  do { \
    double *temp = vm.stack_top-2; \
    *temp = *temp op *(temp+1); \
    --vm.stack_top; \
  } while (false)

  /*
  double b = pop(); \
  double a = pop(); \
  push(a op b); \
  */

  // First instruction is opcode, so we do 'decoding/dispatching' the instruction
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
  printf("          ");
  for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
    printf("[ ");
    print_value(*slot);
    printf(" ]");
  }
  printf("\n");
  disassemble_instruction(vm.chunk,
    (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
      case OP_NEGATE:   negate(); break;
      case OP_RETURN: {
        // Simple instruction for now, which will change later
        print_value(pop());
        printf("\n");
        return INTERPRET_OK;
      }
      default:
        printf("Command '%d' doesn't exist", instruction);
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Chunk *chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}