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

static void clear_stack() {
  vm.stack_top = vm.stack = GROW_ARRAY(Value, vm.stack, vm.capacity, 0);
  vm.capacity = 0;
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // Because incorrect instruction before current
  const size_t instruction = vm.ip - vm.chunk->code - 1;
  const int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  clear_stack();
}

void init_vm() {
  vm.capacity = GROW_CAPACITY(0);
  vm.stack_top = vm.stack = GROW_ARRAY(Value, vm.stack, 0, vm.capacity);

  vm.objects = NULL;
  init_table(&vm.strings);
}

void free_vm() {
  free_table(&vm.strings);
  free_objects();
  clear_stack();
}

void push(const Value value) {
  if (vm.stack_top == vm.stack + vm.capacity) {
    const int old_capacity = vm.capacity;
    vm.capacity = GROW_CAPACITY(old_capacity);
    vm.stack = GROW_ARRAY(Value, vm.stack, old_capacity, vm.capacity);
    vm.stack_top = vm.stack + old_capacity;
  }
  *vm.stack_top++ = value;
}

Value pop() {
  return *--vm.stack_top;
}

static Value peek(const int distance) {
  return vm.stack_top[-1 - distance];
}

static bool is_falsey(const Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  const ObjString *b = AS_STRING(pop());
  const ObjString *a = AS_STRING(pop());
  *vm.stack_top++ = OBJ_VAL((Obj*)string_concat(a, b));
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
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
  for (const Value *slot = vm.stack; slot < vm.stack_top; ++slot) {
    printf("[ ");
    print_value(*slot);
    printf(" ]");
  }
  printf("\n");
  disassemble_instruction(vm.chunk,
    (int)(vm.ip - vm.chunk->code));
#endif

    uint16_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        const Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NIL:   push(NIL_VAL); break;
      case OP_TRUE:  push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_POP:   pop(); break;
      case OP_SET_LOCAL: {
        const uint16_t slot = READ_BYTE();
        vm.stack[slot] = peek(0);
        break;
      }
      case OP_GET_LOCAL: {
        const uint16_t slot = READ_BYTE();
        push(vm.stack[slot]);
        break;
      }
      case OP_GET_GLOBAL: {
        const ObjString *name = READ_STRING();
        uint16_t ind; // plug
        const GlobalVar *var = global_find(&vm.globals, name, &ind);
        if (var == NULL) {
          runtime_error("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(var->value);
        break;
      }
      // Bytecode representation: 5, true, name, where 5 is value, bool is constant or not
      case OP_DEFINE_GLOBAL: { // 3 bytes opcode: define, string and constant
        const bool constant = AS_BOOL(pop());
        const ObjString *name = READ_STRING();

        // Warn! Don't pop in set, because of future garbage collector work
        global_set(&vm.globals, name, peek(0), constant);
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        const ObjString *name = READ_STRING();
        uint16_t ind;
        const GlobalVar *var = global_find(&vm.globals, name, &ind);

        if (var == NULL) {
          runtime_error("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        if (var->constant) {
          runtime_error("Can't reassign a constant '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        global_set_at(&vm.globals, peek(0), ind);
        break;
      }
      case OP_EQUAL: {
        const Value b = pop();
        const Value a = pop();
        *vm.stack_top++ = BOOL_VAL(values_equal(a, b));
        break;
      }
      case OP_NOT_EQUAL: {
        const Value b = pop();
        const Value a = pop();
        *vm.stack_top++ = BOOL_VAL(!values_equal(a, b));
        break;
      }
      case OP_GREATER:       BINARY_OP(BOOL_VAL, >);  break;
      case OP_GREATER_EQUAL: BINARY_OP(BOOL_VAL, >=); break;
      case OP_LESS:          BINARY_OP(BOOL_VAL, <);  break;
      case OP_LESS_EQUAL:    BINARY_OP(BOOL_VAL, <=); break;
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
      case OP_PRINT: {
        print_value(pop());
        printf("\n");
        break;
      }
      case OP_RETURN: {
        // Exit interpreter
        return INTERPRET_OK;
      }
      default:
        printf("Command '%d' doesn't exist", instruction);
        runtime_error("Programming language problem.");
        return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
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