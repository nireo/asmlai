#include "compiler.h"
#include "ast.h"
#include "codegen.h"
#include "token.h"
#include <cstdio>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, bool> global_symbols;

int
compile_ast_node(const Node &node)
{
  switch(node.Type()) {
  case AstType::Program: {
    int last = -1;
    const auto &program = static_cast<const Program &>(node);

    for(const auto &stmt : program.statements_) {
      last = compile_ast_node(*stmt);
    }

    if(last == -1) {
      std::fprintf(stderr, "something went wrong in the execution");
      std::exit(1);
    }

    return last;
  }
  case AstType::ExpressionStatement: {
    const auto &expr_stmt = static_cast<const ExpressionStatement &>(node);
    return compile_ast_node(*expr_stmt.expression_);
  }
  case AstType::InfixExpression: {
    const auto &infix_exp = static_cast<const InfixExpression &>(node);

    if(infix_exp.left_ != nullptr && infix_exp.right_ == nullptr) {
      return compile_ast_node(*infix_exp.left_);
    }

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
    case tokentypes::LT: {
      return codegen_lt(left, right);
    }
    case tokentypes::GT: {
      return codegen_gt(left, right);
    }
    case tokentypes::Eq: {
      return codegen_equal(left, right);
    }
    case tokentypes::Neq: {
      return codegen_nequal(left, right);
    }
    default: {
      std::fprintf(stderr, "unknown operator type type\n");
      std::exit(1);
    }
    }
  }
  case AstType::LetStatement: {
    const auto &assigment = static_cast<const LetStatement &>(node);
    const auto &identifier = static_cast<const Identifier &>(*assigment.name_);

    global_symbols[identifier.value_] = true;
    generate_sym(identifier.value_);

    int reg = compile_ast_node(*assigment.value_);
    return  store_global(reg, identifier.value_);
  }
  case AstType::Identifier: {
    const auto &identifier = static_cast<const Identifier &>(node);

    if (global_symbols.find(identifier.value_) == global_symbols.end()) {
      std::fprintf(stderr, "identifier '%s' not found\n", identifier.value_.c_str());
      std::exit(1);
    }

    return load_global(identifier.value_);
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
