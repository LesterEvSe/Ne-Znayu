#ifndef PL_OBJECT_H
#define PL_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_STRING(value)    is_obj_type(value, OBJ_STRING)

#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

// Use intrusive list here.
// This is similar to linked list, but not an add-on
struct Obj {
  ObjType type;
  struct Obj *next;
};

// Here, we can safely cast ObjString* to Obj*
// Because their initial memory is the same
struct ObjString {
  Obj obj;
  int length;
  uint32_t hash;

  // Flexible Array Member (FAM) optimization - https://en.wikipedia.org/wiki/Flexible_array_member
  // Must be the last element of the struct
  char chars[];
};

ObjString *string_concat(const ObjString *a, const ObjString *b);

// TODO maybe delete later
// ObjString *take_string(char *chars, int length); // take ownership
ObjString *copy_string(const char *chars, int length); // copy
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // PL_OBJECT_H