#include "codegen.h"
#include "parser.h"
#include "types.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <variant>

namespace codegen {
static constexpr const char *argument_registers[6] = {"%rdi", "%rsi", "%rdx",
                                                      "%rcx", "%r8",  "%r9"};
static i64 depth{};

static i64 count() {
  static i64 i = i;
  return i++;
}

static i64 align_to(i64 n, i64 align) {
  return (n + align - 1) / align * align;
}

template <typename... Args> static void emit(const char *fmt, Args... args) {
  printf("  ");
  printf(fmt, args...);
  printf("\n");
}

static void push() {
  emit("push %%rax");
  ++depth;
}

static void pop(const char *argument) {
  emit("pop %s", argument);
  --depth;
}

static void gen_expression(const parser::Node &node);
static void gen_address(const parser::Node &node) {
  if (node.type_ == parser::NodeType::Variable) {
    int offset = std::get<std::shared_ptr<parser::Object>>(node.data_)->offset_;
    emit("lea %d(%%rbp), %%rax", offset);
    return;
  } else if (node.type_ == parser::NodeType::Derefence) {
    gen_expression(*node.lhs_);
    return;
  }

  std::fprintf(stderr, "non-lvalue\n");
}

static void assign_lvar_offsets(parser::Function &func) {
  i64 offset = 0;
  std::reverse(func.locals_.begin(), func.locals_.end());
  for (auto &obj : func.locals_) {
    offset += 8;
    obj->offset_ = -offset;
  }

  func.stack_sz_ = align_to(offset, (i64)16);
}

static void gen_expression(const parser::Node &node) {
  using NodeType = parser::NodeType;
  switch (node.type_) {
  case NodeType::Num: {
    emit("mov $%ld, %%rax", std::get<i64>(node.data_));
    return;
  }
  case NodeType::Neg: {
    gen_expression(*node.lhs_);
    emit("neg %%rax");
    return;
  }
  case NodeType::Variable: {
    gen_address(node);
    emit("mov (%%rax), %%rax");
    return;
  }
  case NodeType::Derefence:
    gen_expression(*node.lhs_);
    emit("mov (%%rax), %%rax");
    return;
  case NodeType::Addr:
    gen_address(*node.lhs_);
    return;
  case NodeType::Assign: {
    gen_address(*node.lhs_);
    push();
    gen_expression(*node.rhs_);
    pop("%rdi");
    emit("mov %%rax, (%%rdi)");
    return;
  }
  case NodeType::FunctionCall: {
    // arguments are stored the same way as body expressions.
    const auto &nodes = std::get<std::vector<parser::NodePtr>>(node.data_);
    i32 arg_count = static_cast<i32>(nodes.size());

    for (const auto &node : nodes) {
      gen_expression(*node);
      push();
    }

    for (i32 i = arg_count - 1; i >= 0; --i) {
      pop(argument_registers[i]);
    }

    emit("mov $0, %%rax");
    emit("call %s", node.func_name_);
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
    emit("add %%rdi, %%rax");
    return;
  }
  case NodeType::Sub: {
    emit("sub %%rdi, %%rax");
    return;
  }
  case NodeType::Mul: {
    emit("imul %%rdi, %%rax");
    return;
  }
  case NodeType::Div: {
    emit("cqo");
    emit("idiv %%rdi");
    return;
  }

  case NodeType::EQ:
  case NodeType::LT:
  case NodeType::NE:
  case NodeType::LE: {
    emit("cmp %%rdi, %%rax");

    if (node.type_ == NodeType::EQ) {
      emit("sete %%al");
    } else if (node.type_ == NodeType::NE) {
      emit("setne %%al");
    } else if (node.type_ == NodeType::LT) {
      emit("setl %%al");
    } else if (node.type_ == NodeType::LE) {
      emit("setle %%al");
    }

    emit("movzb %%al, %%rax");
    return;
  }
  default: {
    std::fprintf(stderr, "invalid node type\n");
  }
  }
}

static void gen_stmt(const parser::Node &node) {

  switch (node.type_) {
  case parser::NodeType::ExprStmt: {
    gen_expression(*node.lhs_);
    return;
  }
  case parser::NodeType::Return: {
    gen_expression(*node.lhs_);
    emit("jmp .L.return");
    return;
  }
  case parser::NodeType::Block: {
    try {
      const auto &nodes =
          std::get<std::vector<std::unique_ptr<parser::Node>>>(node.data_);
      for (const auto &node : nodes) {
        gen_stmt(*node);
      }

      return;
    } catch (const std::bad_variant_access &e) {
      // we have encountered a null statement
      return;
    }
  }
  case parser::NodeType::If: {
    i64 L = count();
    const auto &if_node = std::get<parser::IfNode>(node.data_);
    gen_expression(*if_node.condition_);
    emit("cmp $0, %%rax");
    emit("je .L.else.%ld", L);
    gen_stmt(*if_node.then_);
    emit("jmp .L.end.%ld", L);
    printf(".L.else.%ld:\n", L);
    if (if_node.else_ != nullptr) {
      gen_stmt(*if_node.else_);
    }
    printf(".L.end.%ld:\n", L);
    return;
  }
  case parser::NodeType::For: {
    i64 L = count();
    const auto &for_node = std::get<parser::ForNode>(node.data_);

    if (for_node.initialization_ != nullptr) {
      gen_stmt(*for_node.initialization_);
    }

    printf(".L.begin.%ld:\n", L);
    if (for_node.condition_ != nullptr) {
      gen_expression(*for_node.condition_);
      emit("cmp $0, %%rax");
      emit("je  .L.end.%ld", L);
    }

    gen_stmt(*for_node.body_);
    if (for_node.increment_ != nullptr)
      gen_expression(*for_node.increment_);
    emit("jmp .L.begin.%ld", L);
    printf(".L.end.%ld:\n", L);
    return;
  }
  default: {
    std::fprintf(stderr, "invalid statement\n");
    std::exit(1);
  }
  }
}

void gen_code(parser::Function &root) {
  assign_lvar_offsets(root);

  emit(".globl main");
  printf("main:\n");

  emit("push %%rbp");
  emit("mov %%rsp, %%rbp");
  emit("sub $%ld, %%rsp", root.stack_sz_);

  gen_stmt(*root.body_);
  assert(depth == 0);

  printf(".L.return:\n");
  emit("mov %%rbp, %%rsp");
  emit("pop %%rbp");
  emit("ret");
}
} // namespace codegen
