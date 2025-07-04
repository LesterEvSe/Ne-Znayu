#include <string.h>

#include "common.h"
#include "scanner.h"


// For example statement "print bacon;" here letter b - start, o - current
typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

void init_scanner(const char *source) {
  scanner.start = scanner.current = source;
  scanner.line = 1;
}

static bool is_alpha(const char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_at_end() {
  return *scanner.current == '\0';
}

static char advance() {
  return *scanner.current++;
}

static char peek() {
  return *scanner.current;
}

static char peek_next() {
  if (is_at_end()) return '\0';
  return scanner.current[1];
}

static bool match(char expected) {
  if (is_at_end() || *scanner.current != expected) return false;
  ++scanner.current;
  return true;
}

static Token make_token(const TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token error_token(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

static void skip_whitespace() {
  for (;;) {
    const char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        scanner.line++;
        advance();
        break;
      case '/':  // Think of comments as whitespace

        if (peek_next() == '/') {
          while (peek() != '\n' && !is_at_end()) advance();
        } else if (peek_next() == '*') {
          while((peek() != '*' || peek_next() != '/') && !is_at_end()) advance();
          
          // TODO test and fix multi-line comments
          // Skip */ symbols
          if (!is_at_end()) {
            advance();
            advance();
          }
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static TokenType check_keyword(const int start, const int length,
    const char *rest, const TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static bool is_send() {
  const char *send = "send";

  for (int i = 0; i < 4; ++i)
    if (scanner.current[i] == '\0' || scanner.current[i] != send[i])
      return false;
  
  // Removing an extra point for highlighting. 'send' instead of '.send'
  scanner.start += 1; 
  scanner.current += 4;
  return true;
}

// Tiny trie for our grammar
static TokenType identifier_type() {
  // Or maybe *scanner.start
  switch (scanner.start[0]) {
    case 'a':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'c': return check_keyword(2, 3, "tor", TOKEN_ACTOR);
          case 'n': return check_keyword(2, 1, "d", TOKEN_AND);
        }
      }
      break;
    case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
          case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return check_keyword(1, 3, "end", TOKEN_SEND);  // associated with the is_send function
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
          case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'v':
      if (scanner.current - scanner.start > 2 && scanner.start[1] == 'a') {
        switch (scanner.start[2]) {
          case 'l': return TOKEN_VAL;
          case 'r': return TOKEN_VAR;
        }
      }
      break;
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (is_alpha(peek()) || is_digit(peek())) advance();
  return make_token(identifier_type());
}

static Token number() {
  while (is_digit(peek())) advance();

  // Look for a fractional part
  if (peek() == '.' && is_digit(peek_next())) {
    // Consume the "."
    advance();

    while (is_digit(peek())) advance();
  }

  return make_token(TOKEN_NUMBER);
}

static Token string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') ++scanner.line;
    advance();
  }

  if (is_at_end()) return error_token("unterminated string.");

  // The closing quote.
  advance();
  return make_token(TOKEN_STRING);
}

Token scan_token() {
  skip_whitespace();
  scanner.start = scanner.current;

  if (is_at_end()) return make_token(TOKEN_EOF);

  const char c = advance();
  if (is_alpha(c)) return identifier();
  if (is_digit(c)) return number();

  switch (c) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    case '.':
      return make_token(is_send() ? TOKEN_SEND : TOKEN_DOT);
    case '!':
      return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
    default : return error_token("Unexpected character.");
  }
}