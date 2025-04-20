#ifndef PL_CHUNK_H
#define PL_CHUNK_H

#include "common.h"
#include "value.h"

// Can be expanded over time
typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_GREATER,
  OP_GREATER_EQUAL,
  OP_LESS,
  OP_LESS_EQUAL,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_ACTOR,
  OP_MESSAGE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
} OpCode;

typedef struct {
  int length;
  int capacity;
  uint16_t *code;
  int *lines;  // To track errors
  ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint16_t byte, int line);
int add_constant(Chunk *chunk, Value value);

#endif // PL_CHUNK_H