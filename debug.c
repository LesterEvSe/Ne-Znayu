#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

void disassemble_chunk(const Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->length;) {
    offset = disassemble_instruction(chunk, offset);
  }
}

static int constant_instruction(const char *name, const Chunk *chunk, const int offset) {
  const uint16_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int invoke_instruction(const char *name, const Chunk *chunk, const int offset) {
  const uint16_t constant  = chunk->code[offset + 1];
  const uint16_t arg_count = chunk->code[offset + 2];
  printf("%-16s (%d args) %4d '", name, arg_count, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

static int simple_instruction(const char *name, const int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int byte_instruction(const char *name, const Chunk *chunk, const int offset) {
  const uint16_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int jump_instruction(const char *name, const int sign,
                            const Chunk *chunk, const int offset) {
  uint32_t jump = (chunk->code[offset + 1] << 16);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

// Need to output some additional info about name of variables (for better debugger)
int disassemble_instruction(const Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  const uint16_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:
      return simple_instruction("OP_NIL", offset);
    case OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OP_POP:
      return simple_instruction("OP_POP", offset);
    case OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE:
      return byte_instruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byte_instruction("OP_SET_UPVALUE", chunk, offset);
    case OP_GET_PROPERTY:
      return constant_instruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
      return constant_instruction("OP_SET_PROPERTY", chunk, offset);
    case OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OP_NOT_EQUAL:
      return simple_instruction("OP_NOT_EQUAL", offset);
    case OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OP_GREATER_EQUAL:
      return simple_instruction("OP_GREATER_EQUAL", offset);
    case OP_LESS:
      return simple_instruction("OP_LESS", offset);
    case OP_LESS_EQUAL:
      return simple_instruction("OP_LESS_EQUAL", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_NOT:
      return simple_instruction("OP_NOT", offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OP_JUMP:
      return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jump_instruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byte_instruction("OP_CALL", chunk, offset);
    case OP_INVOKE:
      return invoke_instruction("OP_INVOKE", chunk, offset);
    case OP_CLOSURE: {
      ++offset;
      const uint16_t constant = chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      print_value(chunk->constants.values[constant]);
      printf("\n");

      const ObjFunction *function = AS_FUNCTION(
        chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalue_count; ++j) {
        const int is_local = chunk->code[offset++];
        const int index = chunk->code[offset++];
        printf("%04d      |                     %s %d\n",
               offset - 2, is_local ? "local" : "upvalue", index);
      }

      return offset;
    }
    case OP_ACTOR:
      return constant_instruction("OP_ACTOR", chunk, offset);
    case OP_MESSAGE:
      return constant_instruction("OP_MESSAGE", chunk, offset);
    case OP_CLOSE_UPVALUE:
      return simple_instruction("OP_CLOSE_UPVALUE", offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    default:
      printf("unknown opcode %d\n", instruction);
      return offset + 1;
  }
}