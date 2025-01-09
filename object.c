#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// Warn for using with ObjString, because of FAM
#define ALLOCATE_OBJ(type, object_type) \
  (type*)allocate_object(sizeof(type), object_type)

static Obj *allocate_object(size_t size, ObjType type) {
  Obj *object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;
  return object;
}

/* TODO maybe delete later
static ObjString *allocate_string(char *chars, const int length) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}
*/

/* TODO maybe delete later
ObjString *take_string(char *chars, const int length) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars
  return allocate_string(chars, length);
}
*/

ObjString *string_concat(const ObjString *a, const ObjString *b) {
  const int length = a->length + b->length;
  ObjString *string = (ObjString*)allocate_object(sizeof(ObjString) + length + 1, OBJ_STRING);
  string->length = length;
  memcpy(string->chars, a->chars, a->length);
  memcpy(string->chars + a->length, b->chars, b->length);
  string->chars[length] = '\0';
  return string;
}

ObjString *copy_string(const char *chars, const int length) {
  ObjString *string = (ObjString*)allocate_object(sizeof(ObjString) + length + 1, OBJ_STRING);
  string->length = length;
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  return string;

  /*
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';  // Come in handy, in print_object function
  return allocate_string(heap_chars, length);
  */
}

void print_object(const Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}