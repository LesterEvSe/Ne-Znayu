#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode; // If we have 1 syntax error, do not issue 13 more error related to this
} Parser;

// Order is important!
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

// Some C pointer to function. Ugh...
typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool constant;
} Local;

// Elements ordered in the array in the order that their declarations appear in the code
typedef struct {
  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compiling_chunk;


// Later, logic will be more complicated, so encapsulate this in func
static Chunk *current_chunk() {
  return compiling_chunk;
}

static void error_at(const Token *token, const char *message) {

  // If panic mode set, then ignore other errors related to this
  if (parser.panic_mode) return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing for now
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error(const char *message) {
  error_at(&parser.previous, message);
}

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}

static void consume(const TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static bool check(const TokenType type) {
  return parser.current.type == type;
}

static bool match(const TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emit_byte(const uint8_t byte) {
  write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(const uint8_t byte1, const uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static uint8_t make_constant(const Value value) {
  const int constant = add_constant(current_chunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return constant;
}

static void emit_constant(const Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static void init_compiler(Compiler *compiler) {
  compiler->local_count = compiler->scope_depth = 0;
  current = compiler;
}

static void end_compiler() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disassemble_chunk(current_chunk(), "code");
  }
#endif
}

static void begin_scope() {
  ++current->scope_depth;
}

// TODO can optimize with OP_POPN, to pop several variables at once
static void end_scope() {
  --current->scope_depth;

  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth >
            current->scope_depth) {
    emit_byte(OP_POP);
    --current->local_count;
  }
}

// Several declarations
static void expression();
static void statement();
static void declaration();
static ParseRule *get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static uint8_t identifier_constant(const Token *name) {
  return make_constant(OBJ_VAL((Obj*)copy_string(name->start, name->length)));
}

static bool identifiers_equal(const Token *a, const Token *b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(const Compiler *compiler, const Token *name, bool *constant) {
  for (int i = compiler->local_count - 1; i >= 0; --i) {
    const Local *local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      *constant = local->constant;
      return i;
    }
  }
  return -1;
}

static void add_local(const Token name, const bool constant) {
  if (current->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local *local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
  local->constant = constant;
}

static void declare_variable(const bool constant) {
  if (current->scope_depth == 0) return;

  Token *name = &parser.previous;
  for (int i = current->local_count - 1; i >= 0; --i) {
    const Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break;
    }

    if (identifiers_equal(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

  // Don't worry about passing the local variable, because it's const char* string
  add_local(*name, constant);
}

static uint8_t parse_variable(const char *error_message, const bool constant) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable(constant);
  if (current->scope_depth > 0) return 0;

  return identifier_constant(&parser.previous);
}

// So `val a = a` is error in any case
static void mark_initialized() {
  current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(const uint8_t global, const bool constant) {
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }
  
  emit_constant(BOOL_VAL(constant));
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void binary(const bool can_assign) {
  const TokenType operator_type = parser.previous.type;
  const ParseRule *rule = get_rule(operator_type);
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (operator_type) {
    case TOKEN_BANG_EQUAL:    emit_byte(OP_NOT_EQUAL); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
    case TOKEN_LESS:          emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_byte(OP_LESS_EQUAL); break;
    case TOKEN_PLUS:          emit_byte(OP_ADD);      break;
    case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emit_byte(OP_DIVIDE);   break;
    default:                  return; // Unreachable;
  }
}

static void literal(const bool can_assign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL); break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
    default: return; // Unreachable
  }
}

static void grouping(const bool can_assign) {
  expression(); // takes care of generating bytecode for expr
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number(const bool can_assign) {
  // str to double
  const double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

// If language supported characters like \n or another, then we'd translate those here
static void string(const bool can_assign) {
  emit_constant(OBJ_VAL((Obj*)copy_string(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static void named_variable(const Token name, const bool can_assign) {
  uint8_t get_op, set_op;
  bool constant = false;
  int arg = resolve_local(current, &name, &constant);

  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else {
    arg = identifier_constant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    if (constant) {
      error_at(&name, "Can't reassign a constant");
      return;
    }
    emit_bytes(set_op, (uint8_t)arg);
  } else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}

static void variable(const bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void unary(const bool can_assign) {
  // Write expression to the stack, then negate value in stack
  const TokenType operator_type = parser.previous.type;

  // compile the operand
  parse_precedence(PREC_UNARY);

  // Emit the operator instruction
  switch (operator_type) {
    case TOKEN_BANG:  emit_byte(OP_NOT); break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // Unreachable
  }
}

// Maybe add ?: mixfix operator.
// Explanation: https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_AGENT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

// Starts at the current token and parses any expression at the given precedence level or higher
static void parse_precedence(const Precedence precedence) {
  advance();
  const ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }

  // prefix_rule() code is wrong, because of valid a * b = c + d; expression from language
  // So change it with code below.
  // We need variable `can_assign` only for `variable` function
  const bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  // Process all operators with precedence higher than current
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    const ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment operator.");
  }
}

static ParseRule *get_rule(const TokenType type) {
  return &rules[type];
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// desugars `var a;` into `var a = nil;`
static void var_declaration(const bool constant) {
  const uint8_t global = parse_variable("Expect variable name", constant);

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
  define_variable(global, constant);
}

// expression followed by ; (but not print)
static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

static void print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value");
  emit_byte(OP_PRINT);
}

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_AGENT:
      case TOKEN_FUN:
      case TOKEN_VAL:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        ; // Do nothing
    }

    advance();
  }
}

// TODO can change const and non const variable flow
static void declaration() {
  if (match(TOKEN_VAL)) {
    var_declaration(true);  // constant
  } else if (match(TOKEN_VAR)) {
    var_declaration(false); // variable
  } else {
    statement();
  }

  if (parser.panic_mode) synchronize();
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expression_statement();
  }
}

// Temporary code, work due all other time
bool compile(const char *source, Chunk *chunk) {
  init_scanner(source);
  Compiler compiler;
  init_compiler(&compiler);
  compiling_chunk = chunk;

  parser.had_error = parser.panic_mode = false;
  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  end_compiler();
  return !parser.had_error;
}