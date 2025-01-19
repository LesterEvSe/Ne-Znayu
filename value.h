#ifndef PL_VALUE_H
#define PL_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

// Can be expanded over time
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as;
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(value)    ((Value){VAL_OBJ, {.obj = value}})

// Another dynamic array for Value
// Can be done with generic
typedef struct {
  int length;
  int capacity;
  Value *values;
} ValueArray;

bool values_equal(Value a, Value b);
void init_value_array(ValueArray *array);
void free_value_array(ValueArray *array);
int in_array(const ValueArray *array, Value value); // If not, then -1
void write_value_array(ValueArray *array, Value value);
void print_value(Value value);

#endif // PL_VALUE_H