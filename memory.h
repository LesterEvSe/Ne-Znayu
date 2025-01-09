#ifndef PL_MEMORY_H
#define PL_MEMORY_H

#include "common.h"
#include "object.h"

// We used reallocate everywhere,
// because later it will be convenient to keep track of uncleared memory
#define ALLOCATE(type, count) \
  (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : capacity * 2)

#define GROW_ARRAY(type, pointer, old_size, new_size) \
  (type*)reallocate(pointer, sizeof(type) * (old_size), \
    sizeof(type) * (new_size))

#define FREE_ARRAY(type, pointer, old_size) \
  reallocate(pointer, sizeof(type) * (old_size), 0)

// This function take care of allocating, freeing memory and changing the size
void *reallocate(void *pointer, size_t old_size, size_t new_size);
void free_objects();

#endif // PL_MEMORY_H