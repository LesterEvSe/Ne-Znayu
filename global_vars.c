#include <string.h>
#include "global_vars.h"

bool global_set(GlobalVarArray *arr, const ObjString *name, const Value value, const bool constant) {
  if (arr->length == UINT8_MAX) return false;

  arr->values[arr->length].name = name;
  arr->values[arr->length].value = value;
  arr->values[arr->length++].constant = constant;
  return true;
}

void global_set_at(GlobalVarArray *arr, const Value value, const uint8_t ind) {
  arr->values[ind].value = value;
}

/*
bool global_get(const GlobalVarArray *arr, const ObjString *name, Value *value) {
  return false;
}
*/

/*
const GlobalVar *global_get(const GlobalVarArray *arr, const int ind) {
  return ind < arr->length ? &arr->values[ind] : NULL;
}
*/

const GlobalVar *global_find(const GlobalVarArray *arr, const ObjString *name, uint8_t *ind) {
  for (int i = arr->length-1; i >= 0; --i)
    if (name == arr->values[i].name) { // string interning here too
      *ind = i;
      return &arr->values[i];
    }
  return NULL;
}