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

typedef Node *(*ParseFn)(int can_assign);

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


static Node *number() {
  int value = (int)strtod(parser.previous.start, NULL);
  load_into_register(value);

  return new_node(NULL, NULL, value, AST_INTLITERAL);
}

static Node *binary() { return NULL; }

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {NULL, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static AST_TYPE tokentype_to_ast(TokenType type) {
  switch (type) {
  case TOKEN_PLUS:
    return AST_ADD;
  case TOKEN_MINUS:
    return AST_SUBTRACT;
  case TOKEN_SLASH:
    return AST_DIV;
  case TOKEN_STAR:
    return AST_MULTIPLY;
  default:
    fprintf(stderr, "cannot find ast type for token");
    exit(1);
  }
}

static ParseRule *get_rule(TokenType type) { return &rules[type]; };

static Node *parse_precedence(Precedence prec) {
  Node *right;

  advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("expected expression");
    return NULL;
  }

  int can_assign = prec <= PREC_ASSIGNMENT;
  Node *left = prefix_rule(can_assign);
  TokenType type = parser.current.type;

  while (prec <= get_rule(parser.current.type)->prec) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    Node *right = infix_rule(can_assign);

    left = new_node(left, right, 0, tokentype_to_ast(type));
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("invaldi assignment target");
    return NULL;
  }

  return left;
}


static int code_gen_node(Node *node) {
  int left;
  int right;

  if (node->left)
    left = code_gen_node(node->left);
  if (node->right)
    right = code_gen_node(node->right);

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

static Node *expression() { return parse_precedence(PREC_ASSIGNMENT); }

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "expected ';' after expression");
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    // variable declaration
    Node *tree = expression();
    // generate code for a given tree
    code_gen_node(tree);

    consume(TOKEN_SEMICOLON, "print needs to be followed by print");
  } else {
    expression_statement();
  }
}

void compile_source(const char *source) {
  init_lexer(source);
  parser.had_error = 0;
  parser.panic_mode = 0;

  advance();

  while (!match(TOKEN_EOF)) {
    statement();
  }
}
