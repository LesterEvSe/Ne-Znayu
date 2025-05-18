#ifndef PL_OBJECT_H
#define PL_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

typedef struct VM VM;

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_ACTOR(value)         is_obj_type(value, OBJ_ACTOR)
#define IS_CLOSURE(value)       is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)      is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)      is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value)        is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value)        is_obj_type(value, OBJ_STRING)

#define AS_ACTOR(value)         ((ObjActor*)AS_OBJ(value))
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_INSTANCE(value)      ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_CSTRING(value)       (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_ACTOR,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
} ObjType;

// Use intrusive list here.
// This is similar to linked list, but not an add-on
struct Obj {
  ObjType type;
  bool is_marked;
  Obj *next;
};

// First class function, so need to be an object
typedef struct {
  Obj obj;
  int arity;
  int upvalue_count;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value* (*NativeFn)(int arg_count, Value *args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

// Here, we can safely cast ObjString* or ObjFunction* to Obj*
// Because their initial memory is the same (Starts from Obj)
struct ObjString {
  Obj obj;
  int length;
  uint32_t hash;

  // Flexible Array Member (FAM) optimization - https://en.wikipedia.org/wiki/Flexible_array_member
  // Must be the last element of the struct
  char chars[];
};

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues; // dynamic array of dynamic allocated upvalues
  int upvalue_count; // has it in ObjFunction, but need also here for GC (Garbage Collector)
} ObjClosure;

// TODO queue of messages and VM
typedef struct {
  Obj obj;
  ObjString *name;
  Table messages;
} ObjActor;

typedef struct {
  Obj obj;
  ObjActor *actor;
  Table fields;
} ObjInstance;

ObjActor *new_actor(ObjString *name);
ObjClosure *new_closure(VM *vm, ObjFunction *function);
ObjFunction *new_function(VM *vm);
ObjInstance *new_instance(VM *vm, ObjActor *actor);
ObjNative *new_native(NativeFn function);
ObjString *string_concat(VM *vm, const ObjString *a, const ObjString *b);

// TODO maybe delete later
// ObjString *take_string(char *chars, int length); // take ownership
ObjString *copy_string(VM *vm, const char *chars, int length); // copy
ObjUpvalue *new_upvalue(VM *vm, Value *slot);
void print_object(Value value);

static inline bool is_obj_type(const Value value, const ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // PL_OBJECT_H