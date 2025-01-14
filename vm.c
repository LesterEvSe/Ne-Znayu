#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

// Can be recoded with pointer variable, which pass from main func
VM vm;

// TODO delete later
static void reset_stack() {
  vm.capacity = GROW_CAPACITY(0);
  vm.stack_top = vm.stack = GROW_ARRAY(Value, vm.stack, 0, vm.capacity);

  //vm.stack_top = vm.stack;
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  fputs("\n", stderr);

  // Because incorrect instruction before current
  const size_t instruction = vm.ip - vm.chunk->code - 1;
  const int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

void init_vm() {
  reset_stack();
  vm.objects = NULL;
  init_table(&vm.strings);
}

// TODO fix, something went wrong...
void free_vm() {
  free_table(&vm.strings);
  free_objects();
  FREE_ARRAY(Value, vm.stack, vm.capacity);
  reset_stack();
}

void push(Value value) {
  if (vm.stack_top == vm.stack + vm.capacity) {
    int old_capacity = vm.capacity;
    vm.capacity = GROW_CAPACITY(old_capacity);
    vm.stack = GROW_ARRAY(Value, vm.stack, old_capacity, vm.capacity);
    vm.stack_top = vm.stack + old_capacity;
  }
  *vm.stack_top++ = value;
}

Value pop() {
  return *--vm.stack_top;
}

static Value peek(int distance) {
  return vm.stack_top[-1 - distance];
}

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  const ObjString *b = AS_STRING(pop());
  const ObjString *a = AS_STRING(pop());

  *vm.stack_top++ = OBJ_VAL((Obj*)string_concat(a, b));

  /* TODO maybe delete later
  const int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  *vm.stack_top++ = OBJ_VAL((Obj*)take_string(chars, length));
  */
}

void negate() {
  Value *temp = vm.stack_top-1;
  *temp = NUMBER_VAL(-AS_NUMBER(*temp));
}

// TODO in future, need to process the errors
// About 90% of time PL was inside that function.
// Because it's heart of the VM
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(ValueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    *vm.stack_top++ = ValueType(a op b); \
  } while (false)

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
      case OP_NIL:      push(NIL_VAL); break;
      case OP_TRUE:     push(BOOL_VAL(true)); break;
      case OP_FALSE:    push(BOOL_VAL(false)); break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        *vm.stack_top++ = BOOL_VAL(values_equal(a, b));
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          const double b = AS_NUMBER(pop());
          const double a = AS_NUMBER(pop());
          *vm.stack_top++ = NUMBER_VAL(a + b);
        } else {
          runtime_error(
            "Operands must be two numbers or two strings");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        Value *temp = vm.stack_top-1;
        *temp = BOOL_VAL(is_falsey(*temp));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        negate();
        break;
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

InterpretResult interpret(const char *source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  const InterpretResult result = run();

  free_chunk(&chunk);
  return result;
}