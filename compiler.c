#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

// Temporary code
void compile(const char *source) {
  init_scanner(source);
  int line = -1;

  for (;;) {
    Token token = scan_token();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }

    // %.*s - need to restrict token.start string with token.length (because it hasn't '\0')
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if (token.type == TOKEN_EOF) break;
  }
}