#ifndef PL_OBJECT_H
#define PL_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_FUNCTION(value)  is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value)    is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value)    is_obj_type(value, OBJ_STRING)

#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_NATIVE(value)    (((ObjNative*)AS_OBJ(value))->function)
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
} ObjType;

// Use intrusive list here.
// This is similar to linked list, but not an add-on
struct Obj {
  ObjType type;
  Obj *next;
};

// First class function, so need to be an object
typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int arg_count, Value *args);

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

ObjFunction *new_function();
ObjNative *new_native(NativeFn function);
ObjString *string_concat(const ObjString *a, const ObjString *b);

// TODO maybe delete later
// ObjString *take_string(char *chars, int length); // take ownership
ObjString *copy_string(const char *chars, int length); // copy
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // PL_OBJECT_H