#include "lexer.h"

Lexer lex;

static int match(char exp) {
  if (*lex.current == '\0')
    return 0;

  if (*lex.current != exp)
    return 0;

  ++lex.current;
  return 1;
}

static char next() {
  // take last character and return it
  ++lex.current;
  return lex.current[-1];
}

static Token take_token(TokenType type) {
  Token tok;
  tok.type = type;
  tok.start = lex.start;
  tok.len = (int)(lex.current - lex.start);
  tok.line = lex.line;

  return tok;
}

static char peek() { return *lex.current; }
static char peek_next() {
  if (*lex.current == '\0')
    return '\0';

  return lex.current[1];
}
