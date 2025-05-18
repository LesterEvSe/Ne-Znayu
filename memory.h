#ifndef PL_MEMORY_H
#define PL_MEMORY_H

#include "common.h"
#include "object.h"
#include "vm.h"

// We used reallocate everywhere,
// because later it will be convenient to keep track of uncleared memory
#define ALLOCATE(vm, type, count) \
  (type*)reallocate(vm, NULL, 0, sizeof(type) * (count))

#define FREE(vm, type, pointer) reallocate(vm, pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : capacity * 2)

#define GROW_ARRAY(vm, type, pointer, old_size, new_size) \
  (type*)reallocate(vm, pointer, sizeof(type) * (old_size), \
    sizeof(type) * (new_size))

#define FREE_ARRAY(vm, type, pointer, old_size) \
  reallocate(vm, pointer, sizeof(type) * (old_size), 0)

// This function take care of allocating, freeing memory and changing the size
void *reallocate(VM *vm, void *pointer, size_t old_size, size_t new_size);
void mark_object(VM *vm, Obj *object);
void mark_value(VM *vm, Value value);
void mark_table(VM *vm, Table *table);
void table_remove_white(VM *vm, Table *table);
void collect_garbage(VM *vm);
void free_objects(VM *vm);

#endif // PL_MEMORY_H