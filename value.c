#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void init_value_array(ValueArray *array) {
  array->length = 0;
  array->capacity = 0;
  array->values = NULL;
}

void free_value_array(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}

void write_value_array(ValueArray *array, const Value value) {
  if (array->capacity < array->length + 1) {
    const int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
  }
  array->values[array->length++] = value;
}

void print_value(const Value value) {
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    case VAL_OBJ: print_object(value); break;
    default: return; // Unreachable
  }
}

bool values_equal(const Value a, const Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
    default:         return false; // Unreachable
  }
}

int in_array(const ValueArray *array, const Value value) {
  for (int i = 0; i < array->length; ++i)
    if (values_equal(array->values[i], value))
      return i;
  return -1;
}