#include <string.h>
#include "global_vars.h"

bool global_set(GlobalVarArray *arr, const ObjString *name, const Value value, const bool constant) {
  if (arr->length == UINT16_MAX) return false;

  arr->values[arr->length].name = name;
  arr->values[arr->length].value = value;
  arr->values[arr->length++].constant = constant;
  return true;
}

void global_set_at(GlobalVarArray *arr, const Value value, const uint16_t ind) {
  arr->values[ind].value = value;
}

const GlobalVar *global_find(const GlobalVarArray *arr, const ObjString *name, uint16_t *ind) {
  for (int i = arr->length-1; i >= 0; --i)
    if (name == arr->values[i].name) { // string interning here too
      *ind = i;
      return &arr->values[i];
    }
  return NULL;
}