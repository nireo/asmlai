#include "compiler.h"
#include "ast.h"
#include "codegen_x64.h"
#include "token.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, Symbol> global_symbols;

void
add_new_symbol(const std::string &name, const symboltype stype,
               const valuetype vtype)
{
  std::cout << "added symbol: " << name << '\n';

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

  return false;
}

static bool
number_type(const valuetype type)
{
  if(type == TYPE_CHAR || type == TYPE_INT || type == TYPE_LONG)
    return true;

  return false;
}

static bool
is_ptr_type(const valuetype type)
{

  if(type == TYPE_PTR_CHAR || type == TYPE_PTR_INT || type == TYPE_PTR_LONG
     || type == TYPE_PTR_VOID) {
    return true;
  }

  return false;
}

static const valuetype
convert_pointer_to_normal(const valuetype type)
{
  switch(type) {
  case TYPE_PTR_VOID:
    return TYPE_VOID;
  case TYPE_PTR_CHAR:
    return TYPE_CHAR;
  case TYPE_PTR_INT:
    return TYPE_INT;
  case TYPE_PTR_LONG:
    return TYPE_LONG;
  default: {
    std::fprintf(stderr, "type is not convertable to normal type.");
    std::exit(1);
  }
  }
}

std::unique_ptr<Expression>
change_type(std::unique_ptr<Expression> exp, valuetype change_type)
{
  auto exp_type = exp->ValueType();
  if(number_type(change_type) && number_type(exp_type)) {
    if(change_type == exp->ValueType())
      return std::move(exp);

    int size_1 = get_bytesize_of_type(exp_type);
    int size_2 = get_bytesize_of_type(change_type);

    if(size_1 > size_2)
      return nullptr;

    if(size_2 > size_1) {
      // TODO: widen the expression
      return std::move(exp);
    }
  }

  if(is_ptr_type(exp_type)) {
    if(exp->Type() != AstType::InfixExpression && exp_type == change_type)
      return std::move(exp);
  }

  if(exp->Type() == AstType::InfixExpression) {
    const auto &infix = static_cast<const InfixExpression &>(*exp);
    if(infix.opr == tokentypes::Plus && infix.opr == tokentypes::Minus) {
      if(number_type(exp_type) && is_ptr_type(change_type)) {
        int size = get_bytesize_of_type(convert_pointer_to_normal(change_type));
        if(size > 1) {
          auto wrapper = std::make_unique<TypeChangeAction>();
          wrapper->size = size;
          wrapper->inner_ = std::move(exp);
          wrapper->action_ = TypeChange::Scale;

          return std::move(wrapper);
        }
      }
    }
  }

  // type cannot be change, and thus is not compatible
  return nullptr;
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
      const auto &sym = get_symbol(identifier.value_);
      generate_sym(sym);
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
  case AstType::TypeChangeAction: {
    const auto &tca = static_cast<const TypeChangeAction &>(node);
    switch(tca.action_) {
    case TypeChange::Widen:
      return compile_ast_node(*tca.inner_, -1, node.Type());
    case TypeChange::Scale: {
      int left = compile_ast_node(*tca.inner_, -1, node.Type());
      int right = codegen_load_int(0);

      return mul_registers(left, right);
    }
    }

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
  case AstType::GlobalStatement: {
    const auto &globl = static_cast<const GlobalVariable&>(node);
    const auto &ident = static_cast<const Identifier&>(*globl.identifier_);

    // parser has already generated symbol
    const auto &sym = get_symbol(ident.value_);
    generate_sym(sym);

    return -1;
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
