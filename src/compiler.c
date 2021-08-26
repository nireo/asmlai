#include "compiler.h"
#include "ast.h"
#include "code_gen.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_SUBSCRIPT,  // []
  PREC_PRIMARY
} Precedence;

typedef struct {
  Token current;
  Token previous;
  int had_error;
  int panic_mode;
} Parser;

typedef void (*ParseFn)(int can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence prec;
} ParseRule;

Parser parser;

static void error_at(Token *token, const char *message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = 1;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
  } else {
    fprintf(stderr, " at '%.*s'", token->len, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = 1;
}

static void error(const char *message) { error_at(&parser.previous, message); }

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = get_token();
    if (parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}

static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static int check(TokenType type) { return parser.current.type == type; }
static int match(TokenType type) {
  if (!check(type))
    return 0;

  advance();

  return 1;
}

static int code_gen_node(Node *node, int reg) {
  int left;
  int right;

  if (node->left)
    left = code_gen_node(node->left, -1);
  if (node->right)
    right = code_gen_node(node->right, left);

  switch (node->type) {
  case AST_ADD:
    return add_registers(left, right);
  case AST_SUBTRACT:
    return sub_registers(left, right);
  default:
    fprintf(stderr, "unknown node type\n");
    exit(1);
  }
}

void compile_source(const char *source) {
  init_lexer(source);
  parser.had_error = 0;
  parser.panic_mode = 0;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }
}
