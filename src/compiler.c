#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

int compile_node(Node *node) {
  int left;
  int right;

  if (node->left)
    left = compile_node(node->left);

  if (node->right)
    right = compile_node(node->right);

  switch (node->type) {
  case AST_ADD:
    return 0;
  case AST_SUBTRACT:
    return 0;
  case AST_MULTIPLY:
    return 0;
  case AST_DIV:
    return 0;
  case AST_INT:
    return 0;
  default:
    fprintf(stderr, "unknown type");
    exit(1);
  }
}
