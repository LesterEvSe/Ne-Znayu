#include <string.h>
#include "global_vars.h"
#include "memory.h"

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

void mark_globals(GlobalVarArray *arr) {
  for (int i = 0; i < arr->length; ++i) {
    GlobalVar *var = &arr->values[i];
    mark_object((VM*)&main_vm, (Obj*)var->name);
    mark_value((VM*)&main_vm, var->value);
  }
}