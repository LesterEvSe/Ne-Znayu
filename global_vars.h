#ifndef PL_GLOBAL_VARS
#define PL_GLOBAL_VARS

#include "common.h"
#include "value.h"

typedef struct {
    const ObjString *name;
    Value value;
    bool constant;
} GlobalVar;

typedef struct {
    int length;
    GlobalVar values[UINT16_COUNT];
} GlobalVarArray;

bool global_set(GlobalVarArray *arr, const ObjString *name, Value value, bool constant);
void global_set_at(GlobalVarArray *arr, Value value, uint16_t ind);
const GlobalVar *global_find(const GlobalVarArray *arr, const ObjString *name, uint16_t *ind);

#endif // PL_GLOBAL_VARS