#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// For native func
#include <time.h>
#include <math.h>

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

// Can be recoded with pointer variable, which pass from main func
VM vm;

static void runtime_error(const char *format, ...);

// Native functions
// TODO
// 1. Move to separate file
// 2. Add input stream with (scanf for example)
// 3. Add input/output streams (with files)
static Value *clock_native(const int arg_count, Value *args) {
  if (arg_count != 0) {
    runtime_error("Expect 0 arguments in clock function but got %d.", arg_count);
    return NULL;
  }

  Value *res = NULL;
  res = (Value*)reallocate(res, 0, sizeof(Value));
  *res = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
  return res;
}

static Value *sqrt_native(const int arg_count, Value *args) {
  if (arg_count != 1) {
    runtime_error("Expect 1 argument in sqrt function but got %d.", arg_count);
    return NULL;
  }

  // TODO check with actors or funcs
  if (args[0].type == VAL_OBJ) {
    runtime_error("First argument is not a number.");
    return NULL;
  }

  Value *res = NULL;
  res = (Value*)reallocate(res, 0, sizeof(Value));

  if (args[0].type == VAL_BOOL) {
    *res = NUMBER_VAL(sqrt(AS_BOOL(args[0])));
  } else { // Nil or number
    *res = NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
  }
  return res;
}


static void clear_stack() {
  vm.stack_top = vm.stack = FREE_ARRAY(Value, vm.stack, vm.capacity); // GROW_ARRAY(Value, vm.stack, vm.capacity, 0);
  vm.capacity = vm.frame_count = 0;
  vm.open_upvalues = NULL;
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // Have all stack trace, so use it to show more clear errors!
  // Opposite to python style, first of all exact place of error, then stack trace
  for (int i = vm.frame_count - 1; i >= 0; --i) {
    const CallFrame *frame = &vm.frames[i];
    const ObjFunction *function = frame->closure->function;
    const size_t instruction = frame->ip - function->chunk.code - 1;

    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  clear_stack();
}

static void define_native(const char *name, const NativeFn function) {
  push(OBJ_VAL((Obj*)copy_string(name, (int)strlen(name))));
  push(OBJ_VAL((Obj*)new_native(function)));

  // Buggy one. double free corruption in recursive func :D
  global_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1], true);
  pop();
  pop();
}

void init_vm() {
  vm.frame_count = 0;
  vm.capacity = GROW_CAPACITY(0);
  vm.stack = NULL;  // To prevent UB from compiler
  vm.stack_top = vm.stack = GROW_ARRAY(Value, vm.stack, 0, vm.capacity);

  vm.objects = NULL;
  vm.open_upvalues = NULL;

  vm.bytes_allocated = 0;
  vm.next_gc = 1024 * 1024; // Some random number. For real language, need to tune better

  vm.gray_count = 0;
  vm.gray_capacity = 0;
  vm.gray_stack = NULL;

  init_table(&vm.strings);

  // Because during copy_string, GC can be called
  vm.init_string = NULL;
  vm.init_string = copy_string("init", 4);

  define_native("clock", clock_native);
  define_native("sqrt", sqrt_native);
}

void free_vm() {
  free_table(&vm.strings);
  vm.init_string = NULL;
  free_objects();
  clear_stack();
}

void push(const Value value) {
  if (vm.stack_top == vm.stack + vm.capacity) {
    const int old_capacity = vm.capacity;
    vm.capacity = GROW_CAPACITY(old_capacity);
    const Value *stack = vm.stack;
    vm.stack = GROW_ARRAY(Value, vm.stack, old_capacity, vm.capacity);
    vm.stack_top = vm.stack + old_capacity;

    // 3 days of debug...
    for (int i = vm.frame_count - 1; i >= 0; --i) {
      vm.frames[i].slots = vm.stack + (vm.frames[i].slots - stack);
    }
  }
  *vm.stack_top++ = value;
}

Value pop() {
  return *--vm.stack_top;
}

static Value peek(const int distance) {
  return vm.stack_top[-1 - distance];
}

// TODO delete later, because we have dynamically typed language
static bool call(ObjClosure *closure, const int arg_count) {
  if (arg_count != closure->function->arity) {
    runtime_error("Expect %d arguments but got %d.",
      closure->function->arity, arg_count);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;

  // Maybe bug here, when we expand memory, then we drop vm.stack_top and here we have invalid...
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}

static bool call_value(const Value callee, const int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_MESSAGE: {
        ObjBoundMessage *bound = AS_BOUND_MESSAGE(callee);
        vm.stack_top[-arg_count - 1] = bound->receiver;
        return call(bound->message, arg_count);
      }
      case OBJ_ACTOR: {  // Initializer or error
        ObjActor *actor = AS_ACTOR(callee);
        vm.stack_top[-arg_count - 1] = OBJ_VAL((Obj*)new_instance(actor));
        Value initializer;

        if (table_get(&actor->messages, vm.init_string, &initializer)) {
          return call(AS_CLOSURE(initializer), arg_count);
        } else if (arg_count != 0) {
          runtime_error("Expected 0 arguments but got %d.", arg_count);
          return false;
        }
        return true;
      }
      // All OBJ_FUNCTION inside closures
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), arg_count);
      case OBJ_NATIVE: {
        const NativeFn native = AS_NATIVE(callee);
        Value *result = native(arg_count, vm.stack_top - arg_count);
        if (result == NULL) return false;

        vm.stack_top -= arg_count + 1;
        push(*result);
        reallocate(result, sizeof(Value), 0);
        return true;
      }
      default:
        break; // Non-callable object type
    }
  }

  runtime_error("Can only call functions and actors.");
  return false;
}

// Combines OP_GET_PROPERTY and OP_CALL
static bool invoke_from_actor(const ObjActor *actor, const ObjString *name, const int arg_count) {
  Value message;
  if (!table_get(&actor->messages, name, &message)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }

  return call(AS_CLOSURE(message), arg_count);
}

static bool invoke(const ObjString *name, const int arg_count) {
  const Value receiver = peek(arg_count);

  if (!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have messages.");
    return false;
  }

  const ObjInstance *instance = AS_INSTANCE(receiver);
  return invoke_from_actor(instance->actor, name, arg_count);
}

static bool bind_message(ObjActor *actor, ObjString *name) {
  Value message;
  if (!table_get(&actor->messages, name, &message)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMessage *bound = new_bound_message(peek(0), AS_CLOSURE(message));
  pop();
  push(OBJ_VAL((Obj*)bound));
  return true;
}

// TODO can be recoded to more elegant way with pointers
static ObjUpvalue *capture_upvalue(Value *local) {
  ObjUpvalue *prev_upvalue = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue *created_upvalue = new_upvalue(local);
  created_upvalue->next = upvalue;

  if (prev_upvalue == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }

  return created_upvalue;
}

static void close_upvalues(const Value *last) {
  while (vm.open_upvalues != NULL &&
         vm.open_upvalues->location >= last) {
    ObjUpvalue *upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

// The VM trust own compiler, because only way to start it with compiler,
// so we do not make any checks
static void define_message(ObjString *name) {
  Value message = peek(0);
  ObjActor *actor = AS_ACTOR(peek(1));
  table_set(&actor->messages, name, message);
  pop();
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
  CallFrame *frame = &vm.frames[vm.frame_count - 1];
#define READ_WORD() (*frame->ip++)
#define READ_INT() (frame->ip += 2, (uint32_t)((frame->ip[-2] << 16) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_WORD()])
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
  disassemble_instruction(&frame->closure->function->chunk,
    (int)(frame->ip - frame->closure->function->chunk.code));
#endif

    uint16_t instruction;
    switch (instruction = READ_WORD()) {
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
        const uint16_t slot = READ_WORD();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_LOCAL: {
        const uint16_t slot = READ_WORD();
        push(frame->slots[slot]);
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
      case OP_GET_UPVALUE: {
        const uint16_t slot = READ_WORD();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      // If this slow, then everything is slow
      case OP_SET_UPVALUE: {
        const uint16_t slot = READ_WORD();
        // Closure is indirection above function (maybe, :D)
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtime_error("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance *instance = AS_INSTANCE(peek(0));
        ObjString *name = READ_STRING();

        Value value;
        if (table_get(&instance->fields, name, &value)) {
          pop(); // Instance
          push(value);
          break;
        }
        
        if (!bind_message(instance->actor, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtime_error("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }
        
        ObjInstance *instance = AS_INSTANCE(peek(1));
        table_set(&instance->fields, READ_STRING(), peek(0));
        Value value = pop();
        pop();
        push(value);
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
        push(BOOL_VAL(is_falsey(pop())));
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
      case OP_JUMP: {
        const uint32_t offset = READ_INT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        const uint32_t offset = READ_INT();
        if (is_falsey(peek(0))) frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        const uint32_t offset = READ_INT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        const int arg_count = READ_WORD();
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
      case OP_INVOKE: {
        const ObjString *message = READ_STRING();
        const int arg_count = READ_WORD();
        if (!invoke(message, arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure *closure = new_closure(function);
        push(OBJ_VAL((Obj*)closure));

        for (int i = 0; i < closure->upvalue_count; ++i) {
          const uint16_t is_local = READ_WORD();
          const uint16_t index = READ_WORD();

          if (is_local) {
            closure->upvalues[i] = capture_upvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_ACTOR:
        push(OBJ_VAL((Obj*)new_actor(READ_STRING())));
        break;
      case OP_MESSAGE:
        define_message(READ_STRING());
        break;
      case OP_CLOSE_UPVALUE:
        close_upvalues(vm.stack_top - 1);
        pop();
        break;
      case OP_RETURN: {
        const Value result = pop();
        close_upvalues(frame->slots);
        if (--vm.frame_count == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stack_top = frame->slots;
        push(result);
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
      default:
        printf("Command '%d' doesn't exist", instruction);
        runtime_error("Programming language problem.");
        return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef READ_WORD
#undef READ_INT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
  ObjFunction *function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL((Obj*)function));
  ObjClosure *closure = new_closure(function);
  pop();
  push(OBJ_VAL((Obj*)closure));
  call(closure, 0);

  return run();
}