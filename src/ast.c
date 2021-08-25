#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

Node *new_node(Node *left, Node *right, int value, int type) {
  Node *node = (Node*)malloc(sizeof(Node));
  if (node == NULL) {
    fprintf(stderr, "not enough memory.");
    exit(1);
  }

  node->left = left;
  node->right = right;
  node->type = type;
  node->value = value;

  return node;
}
