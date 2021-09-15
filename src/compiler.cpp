#include "compiler.h"
#include "ast.h"
#include "codegen.h"
#include "token.h"
#include <cstdio>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, bool> global_symbols;

static int
label()
{
  static int id = 1;
  return id++;
}

int
compile_ast_node(const Node &node, int reg, const AstType top_type)
{
  switch(node.Type()) {
  case AstType::Program: {
    int last = -1;
    const auto &program = static_cast<const Program &>(node);

    for(const auto &stmt : program.statements_) {
      last = compile_ast_node(*stmt, -1, AstType::Program);
    }

    return last;
  }
  case AstType::BlockStatement: {
    const auto &block = static_cast<const BlockStatement &>(node);

    int last = -1;
    for(const auto &stmt : block.statements_) {
      last = compile_ast_node(*stmt, -1, AstType::BlockStatement);
    }

    return last;
  }
  case AstType::WhileStatement: {
    const auto &while_stmt = static_cast<const WhileStatement &>(node);

    int start_label = label();
    int end_label = label();

    gen_label(start_label);

    compile_ast_node(*while_stmt.cond_, end_label, node.Type());
    free_all_registers();

    compile_ast_node(*while_stmt.body_, -1, node.Type());
    free_all_registers();

    gen_jmp(start_label);
    gen_label(end_label);

    return -1;
  }
  case AstType::IfExpression: {
    const auto &if_stmt = static_cast<const IfExpression &>(node);
    int false_label = label();
    int end_label;
    if(if_stmt.other_ != nullptr) {
      end_label = label();
    }

    compile_ast_node(*if_stmt.cond_, false_label, node.Type());
    free_all_registers();

    compile_ast_node(*if_stmt.after_, -1, node.Type());
    free_all_registers();

    if(if_stmt.other_ != nullptr) {
      gen_jmp(end_label);
    }

    gen_label(false_label);

    if(if_stmt.other_ != nullptr) {
      compile_ast_node(*if_stmt.other_, -1, node.Type());
      free_all_registers();
      gen_label(end_label);
    }

    return 0;
  }
  case AstType::PrintStatement: {
    const auto &print_stmt = static_cast<const PrintStatement &>(node);
    int reg = compile_ast_node(*print_stmt.print_value_, -1, node.Type());

    print_register(reg);
    free_all_registers();

    return -1;
  }
  case AstType::ExpressionStatement: {
    const auto &expr_stmt = static_cast<const ExpressionStatement &>(node);

    compile_ast_node(*expr_stmt.expression_, -1, node.Type());
    return -1;
  }
  case AstType::InfixExpression: {
    const auto &infix_exp = static_cast<const InfixExpression &>(node);

    if(infix_exp.left_ != nullptr && infix_exp.right_ == nullptr) {
      return compile_ast_node(*infix_exp.left_, -1, node.Type());
    }

    int left = 0, right = 0;
    if(infix_exp.left_ != nullptr) {
      left = compile_ast_node(*infix_exp.left_, -1, node.Type());
    }

    if(infix_exp.right_ != nullptr) {
      right = compile_ast_node(*infix_exp.right_, left, node.Type());
    }

    switch(infix_exp.opr) {
    case tokentypes::Plus:
      return add_registers(left, right);
    case tokentypes::Minus:
      return sub_registers(left, right);
    case tokentypes::Asterisk:
      return mul_registers(left, right);
    case tokentypes::Slash:
      return div_registers(left, right);
    case tokentypes::LT:
    case tokentypes::EGT:
    case tokentypes::ELT:
    case tokentypes::GT:
    case tokentypes::Eq:
    case tokentypes::Neq: {
      if(top_type == AstType::IfExpression
         || top_type == AstType::WhileStatement) {
        return codegen_compare_jump(left, right, reg, infix_exp.opr);
      }
      return codegen_compare_no_jump(left, right, infix_exp.opr);
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

    if(global_symbols.find(identifier.value_) == global_symbols.end()) {
      global_symbols[identifier.value_] = true;
      generate_sym(identifier.value_);
    }

    int reg = compile_ast_node(*assigment.value_, -1, node.Type());
    return store_global(reg, identifier.value_);
  }
  case AstType::Identifier: {
    const auto &identifier = static_cast<const Identifier &>(node);

    if(global_symbols.find(identifier.value_) == global_symbols.end()) {
      std::fprintf(stderr, "identifier '%s' not found\n",
                   identifier.value_.c_str());
      std::exit(1);
    }

    return load_global(identifier.value_);
  }
  case AstType::IntegerLiteral: {
    const auto &int_lit = static_cast<const IntegerLiteral &>(node);

    return load_into_register(int_lit.value_);
  }
  default: {
    std::fprintf(stderr, "unknown node type %d\n", node.Type());
    std::exit(1);
  }
  }
}
