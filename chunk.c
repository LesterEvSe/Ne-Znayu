#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {
  chunk->length = chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  init_value_array(&chunk->constants);
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  free_value_array(&chunk->constants);
  init_chunk(chunk);
}

void write_chunk(Chunk *chunk, const uint8_t byte, const int line) {
  if (chunk->capacity < chunk->length + 1) {
    const int old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->length] = byte;
  chunk->lines[chunk->length++] = line;
}

int add_constant(Chunk *chunk, const Value value) {
  const int ind = in_array(&chunk->constants, value);
  if (ind != -1) return ind;

  write_value_array(&chunk->constants, value);
  return chunk->constants.length - 1;
}