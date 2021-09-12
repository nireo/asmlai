#include "compiler.h"
#include "ast.h"
#include "codegen.h"
#include "token.h"
#include <cstdio>

int
compile_ast_node(const Node &node)
{
  switch(node.Type()) {
  case AstType::Program: {
    int last = -1;
    const auto &program = static_cast<const Program&>(node);

    for (const auto& stmt : program.statements_) {
      last = compile_ast_node(*stmt);
    }

    if (last == -1) {
      std::fprintf(stderr, "something went wrong in the execution");
      std::exit(1);
    }

    return last;
  }
  case AstType::ExpressionStatement: {
    const auto &expr_stmt = static_cast<const ExpressionStatement&>(node);
    return compile_ast_node(*expr_stmt.expression_);
  }
  case AstType::InfixExpression: {
    const auto &infix_exp = static_cast<const InfixExpression &>(node);

    int left = 0, right = 0;
    if(infix_exp.left_ != nullptr) {
      left = compile_ast_node(*infix_exp.left_);
    }

    if(infix_exp.right_ != nullptr) {
      right = compile_ast_node(*infix_exp.right_);
    }

    switch(infix_exp.opr) {
    case tokentypes::Plus: {
      return add_registers(left, right);
    }
    case tokentypes::Minus: {
      return sub_registers(left, right);
    }
    case tokentypes::Asterisk: {
      return mul_registers(left, right);
    }
    case tokentypes::Slash: {
      return div_registers(left, right);
    }
    default: {
      std::fprintf(stderr, "unknown operator type type\n");
      std::exit(1);
    }
    }
  }
  case AstType::IntegerLiteral: {
    const auto &int_lit = static_cast<const IntegerLiteral &>(node);

    return load_into_register(int_lit.value_);
  }
  default: {
    std::fprintf(stderr, "unknown node type\n");
    std::exit(1);
  }
  }
}
