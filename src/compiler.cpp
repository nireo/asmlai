#include "compiler.h"
#include "ast.h"
#include "codegen_x64.h"
#include "token.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#define CAST(type, node) static_cast<const type &>(node);

SymbolTable global_symbols;
std::unordered_map<std::string, SymbolTable> function_locals;
static std::string current_function = "";

void create_new_function_table(const std::string &func_name) {
  function_locals[func_name] = SymbolTable();
}

void new_function_local(const std::string &func_name, const std::string &name,
                        ValueT vtype, int label, int size) {
  function_locals[func_name][name] = {
      .name_ = name,
      .st_type = Scope::Local,
      .type_ = TYPE_VARIABLE, // cannot be function
      .value_type_ = vtype,
      .label = label,
      .size = size,
      .position = 0,
  };
}

void new_function_param(const std::string &func_name, const std::string &name,
                        ValueT vtype, int label, int size) {
  function_locals[func_name][name] = {
      .name_ = name,
      .st_type = Scope::Parameter,
      .type_ = TYPE_VARIABLE, // cannot be function
      .value_type_ = vtype,
      .label = label,
      .size = size,
      .position = 0,
  };
}

SymbolTable &get_function_locals(const std::string &name) {
  return function_locals[name];
}

void reset_local_variables() { reset_local_offset(); }

void add_new_symbol(const std::string &name, const symboltype stype,
                    const ValueT vtype) {
  global_symbols[name] = Symbol{
      .name_ = name,
      .type_ = stype,
      .value_type_ = vtype,
      .label = 0,
  };
}

void add_new_symbol(const std::string &name, const symboltype stype,
                    const ValueT vtype, int label) {
  global_symbols[name] = Symbol{
      .name_ = name,
      .st_type = Scope::Global,
      .type_ = stype,
      .value_type_ = vtype,
      .label = label,
      .position = 0,
  };
}

void add_new_symbol(const std::string &name, const symboltype stype,
                    const ValueT vtype, int label, int size) {
  global_symbols[name] = Symbol{.name_ = name,
                                .type_ = stype,
                                .value_type_ = vtype,
                                .label = label,
                                .size = size};
}

const Symbol &get_symbol(const std::string &name) {
  if (current_function != "") {
    if (function_locals[current_function].find(name) !=
        function_locals[current_function].end())
      return function_locals[current_function][name];
  }

  if (global_symbols.find(name) != global_symbols.end()) {
    return global_symbols[name];
  }

  std::fprintf(stderr, "symbol with name '%s' not found\n", name.c_str());
  std::exit(1);
}

const Symbol &get_symbol_w_func(const std::string &func,
                                const std::string &name) {
  if (function_locals[func].find(name) != function_locals[func].end())
    return function_locals[func][name];

  if (global_symbols.find(name) != global_symbols.end()) {
    return global_symbols[name];
  }

  std::fprintf(stderr, "symbol with name '%s' not found\n", name.c_str());
  std::exit(1);
}

Symbol &get_symbol_ref(const std::string &name) {
  auto &symbol = global_symbols[name];
  return symbol;
}

bool symbol_exists(const std::string &name) {
  return global_symbols.find(name) != global_symbols.end();
}

int get_next_label() {
  static int id = 1;
  return id++;
}

bool check_type_compatible(const ValueT left, const ValueT right, bool noleft) {
  if (left == TYPE_VOID || right == TYPE_VOID)
    return false;

  if (left == right)
    return true;

  if (left == TYPE_CHAR && right == TYPE_INT) {
    return true;
  }

  if (left == TYPE_INT && right == TYPE_CHAR) {
    if (noleft)
      return false;
    return true;
  }

  return false;
}

static bool number_type(const ValueT type) {
  if (type == TYPE_CHAR || type == TYPE_INT || type == TYPE_LONG)
    return true;

  return false;
}

static bool is_ptr_type(const ValueT type) {

  if (type == TYPE_PTR_CHAR || type == TYPE_PTR_INT || type == TYPE_PTR_LONG ||
      type == TYPE_PTR_VOID) {
    return true;
  }

  return false;
}

static ValueT convert_pointer_to_normal(const ValueT type) {
  switch (type) {
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

std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>
change_type(std::unique_ptr<Expression> exp, ValueT change_type,
            TokenType infix_opr) {
  auto exp_type = exp->ValueType();
  if (number_type(change_type) && number_type(exp_type)) {
    if (change_type == exp->ValueType())
      return {nullptr, std::move(exp)};

    int size_1 = get_bytesize_of_type(exp_type);
    int size_2 = get_bytesize_of_type(change_type);

    if (size_1 > size_2)
      return {std::move(exp), nullptr};

    if (size_2 > size_1) {
      return {nullptr, std::move(exp)};
    }
  }

  if (is_ptr_type(exp_type)) {
    if (exp->Type() != AstType::InfixExpression && exp_type == change_type)
      return {nullptr, std::move(exp)};
  }

  if (infix_opr == TokenType::Plus || infix_opr == TokenType::Minus) {
    if (number_type(exp_type) && is_ptr_type(change_type)) {
      int size = get_bytesize_of_type(convert_pointer_to_normal(change_type));
      if (size > 1) {
        auto wrapper = std::make_unique<TypeChangeAction>();
        wrapper->size = size;
        wrapper->inner_ = std::move(exp);
        wrapper->action_ = TypeChange::Scale;

        return {nullptr, std::move(wrapper)};
      } else {
        return {std::move(exp), nullptr};
      }
    }
  }

  // type cannot be change, and thus is not compatible
  return {std::move(exp), nullptr};
}

int compile_ast_node(const Node &node, int reg, const AstType top_type) {
  switch (node.Type()) {
  case AstType::Program: {
    const auto &program = CAST(Program, node);

    for (const auto &stmt : program.statements_) {
      compile_ast_node(*stmt, -1, AstType::Program);
      free_all_registers();
    }

    return -1;
  }
  case AstType::BlockStatement: {
    const auto &block = CAST(BlockStatement, node);

    int last = -1;
    for (const auto &stmt : block.statements_) {
      last = compile_ast_node(*stmt, -1, AstType::BlockStatement);
    }

    return last;
  }
  case AstType::WhileStatement: {
    const auto &while_stmt = CAST(WhileStatement, node);

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
  case AstType::StringLiteral: {
    const auto &str_lit = CAST(StringLiteral, node);

    return load_global_str(str_lit.id_);
  }
  case AstType::IfExpression: {
    const auto &if_stmt = CAST(IfExpression, node);
    int false_label = get_next_label();
    int end_label;
    if (if_stmt.other_ != nullptr) {
      end_label = get_next_label();
    }

    compile_ast_node(*if_stmt.cond_, false_label, node.Type());
    free_all_registers();

    compile_ast_node(*if_stmt.after_, -1, node.Type());
    free_all_registers();

    if (if_stmt.other_ != nullptr) {
      gen_jmp(end_label);
    }

    gen_label(false_label);

    if (if_stmt.other_ != nullptr) {
      compile_ast_node(*if_stmt.other_, -1, node.Type());
      free_all_registers();
      gen_label(end_label);
    }

    return -1;
  }
  case AstType::ExpressionStatement: {
    const auto &expr_stmt = CAST(ExpressionStatement, node);

    compile_ast_node(*expr_stmt.expression_, -1, node.Type());
    free_all_registers();
    return -1;
  }
  case AstType::InfixExpression: {
    const auto &infix_exp = CAST(InfixExpression, node);

    if (infix_exp.left_ != nullptr && infix_exp.right_ == nullptr) {
      return compile_ast_node(*infix_exp.left_, -1, node.Type());
    }

    int left = 0, right = 0;
    if (infix_exp.left_ != nullptr) {
      left = compile_ast_node(*infix_exp.left_, -1, node.Type());
    }

    if (infix_exp.right_ != nullptr) {
      right = compile_ast_node(*infix_exp.right_, left, node.Type());
    }

    switch (infix_exp.opr) {
    case TokenType::Plus:
      return add_registers(left, right);
    case TokenType::Minus:
      return sub_registers(left, right);
    case TokenType::Asterisk:
      return mul_registers(left, right);
    case TokenType::Slash:
      return div_registers(left, right);
    case TokenType::LT:
    case TokenType::EGT:
    case TokenType::ELT:
    case TokenType::GT:
    case TokenType::Eq:
    case TokenType::Neq: {
      if (top_type == AstType::IfExpression ||
          top_type == AstType::WhileStatement) {
        return codegen_compare_jump(left, right, reg, infix_exp.opr);
      }
      return codegen_compare_no_jump(left, right, infix_exp.opr);
    }
    case TokenType::RShift: {
      return shift_right_from_reg(left, right);
    }
    case TokenType::LShift: {
      return shift_left_from_reg(left, right);
    }
    case TokenType::LogOr: {
      return codegen_or(left, right);
    }
    case TokenType::LogAnd: {
      return codegen_and(left, right);
    }
    case TokenType::Xor: {
      return codegen_xor(left, right);
    }
    case TokenType::Assign: {
      switch (infix_exp.right_->Type()) {
      case AstType::Identifier: {
        const auto &identifier = CAST(Identifier, *infix_exp.right_);

        if (function_locals[current_function].find(identifier.value_) ==
            function_locals[current_function].end()) {
          return store_global(left, get_symbol(identifier.value_));
        } else {
          const auto &sym =
              function_locals[current_function].at(identifier.value_);
          return store_local(sym, left);
        }
      }
      case AstType::Dereference: {
        return store_dereference(left, right, infix_exp.right_->ValueType());
      }
      default: {
        std::fprintf(stderr, "cannot assign.");
        std::exit(1);
      }
      }
    }
    default: {
      std::fprintf(stderr, "unknown operator type in prefix expression\n");
      std::exit(1);
    }
    }
  }
  case AstType::ReturnStatement: {
    const auto &returnstmt = CAST(ReturnStatement, node);

    int left_reg = compile_ast_node(*returnstmt.return_value_, -1, node.Type());
    codegen_return(left_reg, get_symbol(returnstmt.function_identifier_));

    return -1;
  }
  case AstType::CallExpression: {
    const auto &call_exp = CAST(CallExpression, node);
    const auto &identifier = CAST(Identifier, *call_exp.func_);

    auto sz = (int)call_exp.arguments_.size();
    int size = 1;
    for (int i = sz - 1; i >= 0; --i) {
      int reg = compile_ast_node(*call_exp.arguments_[i], -1,
                                 call_exp.arguments_[i]->Type());
      copy_argument(reg, size++);

      free_all_registers();
    }
    return codegen_call(identifier.value_, size);
  }
  case AstType::AssingmentStatement: {
    const auto &assigment = CAST(AssignmentStatement, node);
    const auto &identifier = CAST(Identifier, *assigment.identifier_);

    int reg = compile_ast_node(*assigment.value_, -1, node.Type());
    return store_global(reg, get_symbol(identifier.value_));
  }
  case AstType::Identifier: {
    const auto &identifier = CAST(Identifier, node);

    std::cout << current_function << '\n';

    if (identifier.rvalue || top_type == AstType::Dereference) {
      if (function_locals[current_function].find(identifier.value_) !=
          function_locals[current_function].end()) {
        const auto &sym = function_locals[current_function][identifier.value_];

        return load_local(sym, TokenType::Eof, false);
      }

      return load_global(get_symbol(identifier.value_), TokenType::Eof, false);
    } else {
      return -1;
    }
  }
  case AstType::FunctionLiteral: {
    const auto &func = CAST(FunctionLiteral, node);
    const auto &name = CAST(Identifier, *func.name_);

    const auto &sym = get_symbol(name.value_);
    current_function = name.value_;
    std::cout << current_function << '\n';

    function_start(name.value_);
    compile_ast_node(*func.body_, -1, node.Type());
    function_end(sym.label);

    return -1;
  }
  case AstType::TypeChangeAction: {
    const auto &tca = CAST(TypeChangeAction, node);
    switch (tca.action_) {
    case TypeChange::Widen:
      return compile_ast_node(*tca.inner_, -1, node.Type());
    case TypeChange::Scale: {
      int left = compile_ast_node(*tca.inner_, -1, node.Type());
      switch (tca.size) {
      case 2:
        return shift_left(left, 1);
      case 4:
        return shift_left(left, 2);
      case 8:
        return shift_left(left, 3);
      }

      int right = codegen_load_int(tca.size);
      return mul_registers(left, right);
    }
    }

    return -1;
  }
  case AstType::PrefixExpression: {
    const auto &pref = CAST(PrefixExpression, node);
    switch (pref.opr) {
    case TokenType::Amper: {
      const auto &identifier = CAST(Identifier, *pref.right_);
      return codegen_addr(get_symbol(identifier.value_));
    }
    case TokenType::Asterisk: {
      int right = compile_ast_node(*pref.right_, -1, node.Type());
      if (pref.right_->is_rvalue()) {
        return codegen_dereference(right, pref.right_->ValueType());
      }

      return right;
    }
    case TokenType::Minus: {
      int right = compile_ast_node(*pref.right_, -1, node.Type());

      return codegen_neg(right);
    }
    case TokenType::Bang: {
      int right = compile_ast_node(*pref.right_, -1, node.Type());

      return codegen_not(right);
    }
    default: {
      std::fprintf(stderr, "cannot codegen for given operation.\n");
      std::exit(1);
    }
    }
  }
  case AstType::GlobalStatement: {
    const auto &globl = CAST(GlobalVariable, node);
    const auto &ident = CAST(Identifier, *globl.identifier_);

    const auto &sym = get_symbol(ident.value_);
    generate_sym(sym);

    return -1;
  }
  case AstType::IntegerLiteral: {
    const auto &int_lit = CAST(IntegerLiteral, node);

    return load_into_register(int_lit.value_);
  }
  case AstType::Addr: {
    const auto &addr_exp = CAST(Addr, node);
    const auto &identifier = CAST(Identifier, *addr_exp.to_addr_);

    return codegen_addr(get_symbol(identifier.value_));
  }
  case AstType::Dereference: {
    const auto &deref = CAST(Dereference, node);
    int compiled = compile_ast_node(*deref.to_dereference_, -1, node.Type());

    if (deref.rvalue) {
      return codegen_dereference(compiled, deref.to_dereference_->ValueType());
    }

    return compile_ast_node(*deref.to_dereference_, -1, node.Type());
  }
  case AstType::VarDecl: {
    return -1;
  }
  case AstType::IdentifierAction: {
    const auto &ident_action = CAST(IdentifierAction, node);
    const auto &identifier = CAST(Identifier, *ident_action.identifier_);

    switch (ident_action.action_) {
    case TokenType::Inc: {
      return load_global(get_symbol(identifier.value_), TokenType::Inc,
                         ident_action.post_);
    }
    case TokenType::Dec: {
      return load_global(get_symbol(identifier.value_), TokenType::Dec,
                         ident_action.post_);
    }
    default:
      std::fprintf(stderr, "unknown identifier action");
      std::exit(1);
    }

    return -1;
  }
  default: {
    std::fprintf(stderr, "unrecognized node type when compiling.\n");
    std::exit(1);
  }
  }
}
