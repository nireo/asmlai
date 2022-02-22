#include "codegen.h"
#include "parser.h"
#include <cassert>
#include <cstdio>

namespace codegen {
static i64 depth{};

static i64 align_to(i64 n, i64 align) {
  return (n + align - 1) / align * align;
}

static void push() {
  printf("  push %%rax\n");
  ++depth;
}

static void pop(const char *argument) {
  printf("  pop %s\n", argument);
  --depth;
}

static void gen_address(const parser::Node &node) {
  if (node.type_ == parser::NodeType::Variable) {
    int offset = std::get<parser::Object>(node.data_).offset_;
    printf("  lea %d(%%rbp), %%rax\n", -offset);
    return;
  }

  std::fprintf(stderr, "non-lvalue\n");
}

static void assign_lvar_offsets(parser::Function &func) {
  i64 offset = 0;
  for (auto &obj : func.locals_) {
    offset += 8;
    obj.offset_ = -offset;
  }
  func.stack_sz_ = align_to(offset, (i64)16);
}

static void gen_expression(const parser::Node &node) {
  using NodeType = parser::NodeType;
  switch (node.type_) {
  case NodeType::Num: {
    printf("  mov $%ld, %%rax\n", std::get<i64>(node.data_));
    return;
  }
  case NodeType::Neg: {
    gen_expression(*node.lhs_);
    printf("  neg %%rax\n");
    return;
  }
  case NodeType::Variable: {
    gen_address(node);
    printf("  mov (%%rax), %%rax\n");
    return;
  }
  case NodeType::Assign: {
    gen_address(*node.lhs_);
    push();
    gen_expression(*node.rhs_);
    pop("%rdi");
    printf("  mov %%rax, (%%rdi)\n");
    return;
  }
  default: {
  }
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

void gen_code(parser::Function &root) {
  assign_lvar_offsets(root);

  printf("  .globl main\n");
  printf("main:\n");

  printf("  push %%rbp\n");
  printf("  mov %%rsp, %%rbp\n");
  printf("  sub $%ld, %%rsp\n", root.stack_sz_);

  for (const auto &node : root.body_) {
    gen_stmt(*node);
    assert(depth == 0);
  }

  printf("  mov %%rbp, %%rsp\n");
  printf("  pop %%rbp\n");
  printf("  ret\n");
}
} // namespace codegen
