#include "compiler.h"
#include "ast.h"
#include "codegen_x64.h"
#include "token.h"
#include <cstdio>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, Symbol> global_symbols;

void
add_new_symbol(const std::string &name, const symboltype stype,
               const valuetype vtype)
{
  global_symbols[name] = Symbol{
    .name_ = name,
    .type_ = stype,
    .value_type_ = vtype,
    .label = 0,
  };
}

void
add_new_symbol(const std::string &name, const symboltype stype,
               const valuetype vtype, int label)
{
  global_symbols[name] = Symbol{
    .name_ = name,
    .type_ = stype,
    .value_type_ = vtype,
    .label = label,
  };
}

const Symbol &
get_symbol(const std::string &name)
{
  if(global_symbols.find(name) == global_symbols.end()) {
    std::fprintf(stderr, "symbol with name '%s' not found", name.c_str());
    std::exit(1);
  }

  return global_symbols[name];
}

Symbol &
get_symbol_ref(const std::string &name)
{
  auto &symbol = global_symbols[name];
  return symbol;
}

bool
symbol_exists(const std::string &name)
{
  return global_symbols.find(name) != global_symbols.end();
}

int
get_next_label()
{
  static int id = 1;
  return id++;
}

bool
check_type_compatible(const valuetype left, const valuetype right, bool noleft)
{
  if(left == TYPE_VOID || right == TYPE_VOID)
    return false;

  if(left == right)
    return true;

  if(left == TYPE_CHAR && right == TYPE_INT) {
    return true;
  }

  if(left == TYPE_INT && right == TYPE_CHAR) {
    if(noleft)
      return false;
    return true;
  }

  return true;
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

    int start_label = get_next_label();
    int end_label = get_next_label();

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
    int false_label = get_next_label();
    int end_label;
    if(if_stmt.other_ != nullptr) {
      end_label = get_next_label();
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

    if(symbol_exists(identifier.value_)) {
      generate_sym(identifier.value_);
    }

    int reg = compile_ast_node(*assigment.value_, -1, node.Type());
    return store_global(reg, get_symbol(identifier.value_));
  }
  case AstType::ReturnStatement: {
    const auto &returnstmt = static_cast<const ReturnStatement &>(node);

    int left_reg
        = compile_ast_node(*returnstmt.return_value_, -1, node.Type());

    codegen_return(left_reg, get_symbol(returnstmt.function_identifier_));

    return -1;
  }
  case AstType::CallExpression: {
    const auto &call_exp = static_cast<const CallExpression &>(node);
    const auto &identifier = static_cast<const Identifier &>(*call_exp.func_);

    int left_reg = compile_ast_node(*call_exp.arguments_[0], -1, node.Type());

    return codegen_call(left_reg, identifier.value_);
  }
  case AstType::AssingmentStatement: {
    const auto &assigment = static_cast<const AssignmentStatement &>(node);
    const auto &identifier
        = static_cast<const Identifier &>(*assigment.identifier_);

    int reg = compile_ast_node(*assigment.value_, -1, node.Type());
    return store_global(reg, get_symbol(identifier.value_));
  }
  case AstType::Identifier: {
    const auto &identifier = static_cast<const Identifier &>(node);

    if(global_symbols.find(identifier.value_) == global_symbols.end()) {
      std::fprintf(stderr, "identifier '%s' not found\n",
                   identifier.value_.c_str());
      std::exit(1);
    }

    return load_global(get_symbol(identifier.value_));
  }
  case AstType::FunctionLiteral: {
    const auto &func = static_cast<const FunctionLiteral &>(node);
    const auto &name = static_cast<const Identifier &>(*func.name_);

    const auto &sym = get_symbol(name.value_);

    function_start(name.value_);
    compile_ast_node(*func.body_, -1, node.Type());
    function_end(sym.label);

    return -1;
  }
  case AstType::PrefixExpression: {
    const auto &pref = static_cast<const PrefixExpression &>(node);
    switch(pref.opr) {
    case tokentypes::Amper: {
      const auto &identifier = static_cast<const Identifier &>(*pref.right_);
      return codegen_addr(get_symbol(identifier.value_));
    }
    case tokentypes::Asterisk: {
      int right = compile_ast_node(*pref.right_, -1, node.Type());
      return codegen_dereference(right, pref.right_->ValueType());
    }
    default: {
      std::fprintf(stderr, "cannot codegen for given operation.\n");
      std::exit(1);
    }
    }
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
