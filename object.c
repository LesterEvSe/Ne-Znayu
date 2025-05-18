#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

// Warn for using with ObjString, because of FAM
#define ALLOCATE_OBJ(vm, type, object_type) \
  (type*)allocate_object(vm, sizeof(type), object_type)

static Obj *allocate_object(VM *vm, const size_t size, const ObjType type) {
  Obj *object = (Obj*)reallocate(vm, NULL, 0, size);
  object->type = type;
  object->is_marked = false;

  object->next = vm->objects;
  vm->objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif
  return object;
}

// TODO Create new VM here, main_vm plug for now
ObjActor *new_actor(ObjString *name) {
  ObjActor *actor = ALLOCATE_OBJ((VM*)&main_vm, ObjActor, OBJ_ACTOR);
  actor->name = name;
  init_table(&actor->messages);
  return actor;
}

ObjClosure *new_closure(VM *vm, ObjFunction *function) {
  ObjUpvalue **upvalues = ALLOCATE(vm, ObjUpvalue*,
                                   function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; ++i) {
    upvalues[i] = NULL;
  }

  ObjClosure *closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

ObjFunction *new_function(VM *vm) {
  ObjFunction *function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
  function->arity = function->upvalue_count = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

ObjInstance *new_instance(VM *vm, ObjActor *actor) {
  ObjInstance *instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
  instance->actor = actor;
  init_table(&instance->fields);
  return instance;
}

ObjNative *new_native(const NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ((VM*)&main_vm, ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

// Maybe the shortest hash
// FNV-1 hash algorithm http://www.isthe.com/chongo/tech/comp/fnv/
static uint32_t hash_string(const char *key, const int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; ++i) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

// TODO maybe somewhere in next two functions can be find bug
ObjString *string_concat(VM *vm, const ObjString *a, const ObjString *b) {
  const int length = a->length + b->length;
  char *temp = ALLOCATE(vm, char, length + 1);
  memcpy(temp, a->chars, a->length);
  memcpy(temp + a->length, b->chars, b->length);
  temp[length] = '\0';
  const uint32_t hash = hash_string(temp, length);

  ObjString *interned = table_find_string(&vm->strings, temp, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(vm, char, temp, length + 1);
    return interned;
  }

  ObjString *string = (ObjString*)allocate_object(vm, sizeof(ObjString) + length + 1, OBJ_STRING);
  string->length = length;
  memcpy(string->chars, temp, length);
  string->chars[length] = '\0';
  string->hash = hash;

  push(OBJ_VAL((Obj*)string));
  table_set(&vm->strings, string, NIL_VAL);
  pop();

  FREE_ARRAY(vm, char, temp, length + 1);
  return string;
}

ObjString *copy_string(VM *vm, const char *chars, const int length) {
  ObjString *string = (ObjString*)allocate_object(vm, sizeof(ObjString) + length + 1, OBJ_STRING);
  string->hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm->strings, chars, length, string->hash);

  if (interned != NULL) {
    // TODO Maybe need to delete next 2 lines
    //vm.objects = vm.objects->next;
    //reallocate(string, sizeof(ObjString) + length + 1, 0);
    return interned;
  }

  string->length = length;
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';

  push(OBJ_VAL((Obj*)string));
  table_set(&vm->strings, string, NIL_VAL);
  pop();

  return string;
}

ObjUpvalue *new_upvalue(VM *vm, Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void print_function(const ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void print_object(const Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_ACTOR:
      printf("%s", AS_ACTOR(value)->name->chars);
      break;
    case OBJ_CLOSURE:
      print_function(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(value));
      break;
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->actor->name->chars);
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    // user can't own this values, but it's need to prevent warning from compiler
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
  }
}