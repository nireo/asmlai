#include "codegen.h"
#include "parser.h"
#include <cassert>
#include <cstdio>

namespace codegen {
static i64 depth{};

static void push() {
  printf("  push %%rax\n");
  ++depth;
}

static void pop(const char *argument) {
  printf("  pop %s\n", argument);
  --depth;
}

static void gen_expression(const parser::Node &node) {
  using NodeType = parser::NodeType;
  if (node.type_ == NodeType::Num) {
    printf("  mov $%ld, %%rax\n", std::get<i64>(node.data_));
    return;
  } else if (node.type_ == NodeType::Neg) {
    gen_expression(*node.lhs_);
    printf("  neg %%rax\n");
    return;
  }

  gen_expression(*node.rhs_);
  push();

  gen_expression(*node.lhs_);
  pop("%rdi");

  switch (node.type_) {
  case NodeType::Add: {
    printf("  add %%rdi, %%rax\n");
    return;
  }
  case NodeType::Sub: {
    printf("  sub %%rdi, %%rax\n");
    return;
  }
  case NodeType::Mul: {
    printf("  imul %%rdi, %%rax\n");
    return;
  }
  case NodeType::Div: {
    printf("  cqo\n");
    printf("  idiv %%rdi\n");
    return;
  }

  case NodeType::EQ:
  case NodeType::LT:
  case NodeType::NE:
  case NodeType::LE: {
    printf("  cmp %%rdi, %%rax\n");

    if (node.type_ == NodeType::EQ) {
      printf("  sete %%al\n");
    } else if (node.type_ == NodeType::NE) {
      printf("  setne %%al\n");
    } else if (node.type_ == NodeType::LT) {
      printf("  setl %%al\n");
    } else if (node.type_ == NodeType::LE) {
      printf("  setle %%al\n");
    }

    printf("  movzb %%al, %%rax\n");
    return;
  }
  default: {
    std::fprintf(stderr, "invalid node type\n");
  }
  }
}

static void gen_stmt(const parser::Node &node) {
  if (node.type_ == parser::NodeType::ExprStmt) {
    gen_expression(*node.lhs_);
    return;
  }

  std::fprintf(stderr, "invalid statement\n");
}

void gen_code(const std::vector<parser::NodePtr> &root) {
  printf("  .globl main\n");
  printf("main:\n");
  for (const auto &stmt : root) {
    gen_stmt(*stmt);
    assert(depth == 0);
  }

  printf("  ret\n");
}
} // namespace codegen
