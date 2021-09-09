#ifndef LAI_AST_H
#define LAI_AST_H

typedef enum {
  AST_ADD,
  AST_SUBTRACT,
  AST_MULTIPLY,
  AST_DIV,
  AST_INTLITERAL,
} AST_TYPE;

typedef struct Node {
  struct Node *left;
  struct Node *right;

  // this needs to be able to hold some different values, but for the beginning
  // it's fine
  int value;
  int type;
  char *str;
} Node;

Node *new_node(Node *left, Node *right, int value, int type);

#endif
