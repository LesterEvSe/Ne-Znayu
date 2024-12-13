#ifndef PL_VALUE_H
#define PL_VALUE_H

#include "common.h"

// Can be expaned over time
typedef double Value;

// Another dynamic array for Value
// Can be done with generic
typedef struct {
  int length;
  int capacity;
  Value *values;
} ValueArray;

void init_value_array(ValueArray *array);
void free_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void print_value(Value value);

#endif // PL_VALUE_H