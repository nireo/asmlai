#include "parser.h"
#include "codegen.h"
#include "token.h"
#include "typesystem.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <variant>

namespace parser {
template <typename... Args> static void error(const char *fmt, Args... args) {
  std::fprintf(stderr, fmt, args...);
  std::fprintf(stderr, "\n");
  std::exit(1);
}

static std::vector<std::shared_ptr<Object>> locals_;
static std::vector<std::shared_ptr<Object>> globals_;

using TokenList = std::vector<token::Token>;
static NodePtr new_node(NodeType type_) {
  auto node = std::make_unique<Node>();
  node->type_ = type_;

  return node;
}

static char *new_unique() {
  static int L_id = 0;
  char *buffer = (char *)malloc(20 * sizeof(char));
  sprintf(buffer, ".L..%d", L_id++);
  return buffer;
}

static void enter_scope() {
  Scope *n = new Scope();
  n->next_ = scopes;
  scopes = n;
}

static void leave_scope() { scopes = scopes->next_; }

static std::shared_ptr<Object> find_var(const token::Token &tok) {
  for (Scope *sc = scopes; sc; sc = sc->next_) {
    for (const auto &v : sc->variables_) {
      if (tok == v.name_) {
        return v.variable_;
      }
    }
  }

  return nullptr;
}

static bool consume(const TokenList &tokens, u64 &pos, const char *str) {
  if (tokens[pos] == str) {
    ++pos;
    return true;
  }

  return false;
}

static void push_scope(char *name, std::shared_ptr<Object> variable) {
  VarScope vscope;
  vscope.name_ = name;
  vscope.variable_ = variable;
  scopes->variables_.push_back(std::move(vscope));
}

static NodePtr new_single(NodeType type_, NodePtr expr) {
  auto node = new_node(type_);
  node->lhs_ = std::move(expr);

  return node;
}

static NodePtr new_binary_node(NodeType type_, NodePtr lhs, NodePtr rhs) {
  auto node = new_node(type_);
  node->lhs_ = std::move(lhs);
  node->rhs_ = std::move(rhs);

  return node;
}

static NodePtr new_variable_node(std::shared_ptr<Object> variable) {
  auto node = new_node(NodeType::Variable);
  node->data_ = std::move(variable);
  return node;
}

static NodePtr new_number(i64 value) {
  auto node = new_node(NodeType::Num);
  node->data_ = value;

  return node;
}

static char *get_identifier(const token::Token &tok) {
  if (tok.type_ != token::TokenType::Identifier) {
    error("expected an identifier");
  }

  return strndup(tok.loc_, tok.len_);
}

static std::shared_ptr<Object> new_var(char *name, Type *ty) {
  std::shared_ptr<Object> obj = std::make_shared<Object>(name, 0);
  obj->is_local_ = false;
  obj->ty_ = ty;
  push_scope(name, obj);

  return obj;
}

// it needs to be a pointer so that we can change the offset stored in the
// object more easily
static std::shared_ptr<Object> new_lvar(char *name, Type *ty) {
  auto obj = new_var(name, ty);
  obj->is_local_ = true;
  obj->ty_ = ty;
  locals_.push_back(obj);

  return obj;
}

static std::shared_ptr<Object> new_gvar(char *name, Type *ty) {
  auto obj = new_var(name, ty);
  obj->is_local_ = false;
  obj->ty_ = ty;
  globals_.push_back(obj);

  return obj;
}

static std::shared_ptr<Object> new_anonymous_variable(Type *ty) {
  return new_gvar(new_unique(), ty);
}

static std::shared_ptr<Object> new_string_literal(char *p, Type *ty) {
  auto var = new_anonymous_variable(ty);
  var->init_data_ = p;

  return var;
}

static void skip_until(const TokenList &tokens, const char *tok, u64 &pos) {
  while (!(tokens[pos] == tok)) {
    ++pos;
  }
  ++pos; // skip the wanted token
}

static NodePtr new_addition(NodePtr lhs_, NodePtr rhs_) {
  auto lhs = std::move(lhs_);
  auto rhs = std::move(rhs_);

  typesystem::add_type(*lhs);
  typesystem::add_type(*rhs);

  // std::cout << "addition on lhs: " << static_cast<int>(lhs->tt_->type_) <<
  // '\n'; std::cout << "addition on rhs: " << static_cast<int>(rhs->tt_->type_)
  // << '\n'

  if (typesystem::is_number(lhs->tt_) && typesystem::is_number(rhs->tt_)) {
    return new_binary_node(NodeType::Add, std::move(lhs), std::move(rhs));
  }

  if (lhs->tt_->base_type_ != nullptr && rhs->tt_->base_type_ != nullptr) {
    error("invalid operands");
    return nullptr;
  }

  if (lhs->tt_->base_type_ == nullptr && rhs->tt_->base_type_ != nullptr) {
    auto tmp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(tmp);
  }

  rhs = new_binary_node(NodeType::Mul, std::move(rhs),
                        new_number(lhs->tt_->base_type_->size_));
  return new_binary_node(NodeType::Add, std::move(lhs), std::move(rhs));
}

static NodePtr new_subtraction(NodePtr lhs_, NodePtr rhs_) {
  auto lhs = std::move(lhs_);
  auto rhs = std::move(rhs_);

  typesystem::add_type(*lhs);
  typesystem::add_type(*rhs);

  if (typesystem::is_number(lhs->tt_) && typesystem::is_number(rhs->tt_)) {
    return new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
  }

  if (lhs->tt_->base_type_ != nullptr && typesystem::is_number(rhs->tt_)) {
    rhs = new_binary_node(NodeType::Mul, std::move(rhs),
                          new_number(lhs->tt_->size_));
    typesystem::add_type(*rhs);

    Type *tt = lhs->tt_;
    auto n = new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
    n->tt_ = tt;
    return n;
  }

  if (lhs->tt_->base_type_ != nullptr && rhs->tt_->base_type_ != nullptr) {
    i32 size = lhs->tt_->size_;
    auto n = new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
    n->tt_ = new parser::Type(Types::Int, kNumberSize);
    return new_binary_node(NodeType::Div, std::move(n), new_number(size));
  }

  error("invalid subtraction operation.");
  return nullptr;
}

static NodePtr parse_compound_stmt(const TokenList &, u64 &);
static NodePtr parse_expression(const TokenList &, u64 &);
static NodePtr parse_equal(const TokenList &, u64 &);
static NodePtr parse_mul(const TokenList &, u64 &);
static NodePtr parse_add(const TokenList &, u64 &);
static NodePtr parse_unary(const TokenList &, u64 &);
static NodePtr parse_primary(const TokenList &, u64 &);
static NodePtr parse_relational(const TokenList &, u64 &);
static NodePtr parse_assign(const TokenList &, u64 &);
static NodePtr parse_postfix(const TokenList &, u64 &);
static Type *parse_struct_declaration(const TokenList &, u64 &);

static Type *declarator(const TokenList &tokens, u64 &pos, Type *ty);

static NodePtr parse_expr_stmt(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == ";") { // encountered a empty statement
    ++pos;
    return new_node(NodeType::Block);
  }

  auto node = new_single(NodeType::ExprStmt, parse_expression(tokens, pos));
  skip_until(tokens, ";", pos);
  return node;
}

static i64 get_number_value(const TokenList &tokens, u64 &pos) {
  if (tokens[pos].type_ != token::TokenType::Num) {
    error("expecting number");
  }

  return std::get<i64>(tokens[pos].data_); // guaranteed not to fail
}

static bool is_typename(const token::Token &tok) {
  return tok == "char" || tok == "int" || tok == "struct";
}

static Type *decl_type(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == "char") {
    ++pos;
    return new Type(Types::Char, kCharSize);
  }

  if (tokens[pos] == "int") {
    ++pos;
    return new Type(Types::Int, kNumberSize);
  }

  if (tokens[pos] == "struct") {
    ++pos;
    return parse_struct_declaration(tokens, pos);
  }

  return nullptr;
}

static Member *struct_members(const TokenList &tokens, u64 &pos) {
  Member head{};
  Member *current = &head;

  while (tokens[pos] != "}") {
    Type *base_type = decl_type(tokens, pos);
    i32 i = 0;

    while (!consume(tokens, pos, ";")) {
      if (i++) {
        skip_until(tokens, ",", pos);
      }

      Member *mem = new Member();
      mem->type = declarator(tokens, pos, base_type);
      mem->name = mem->type->name_;

      current = current->next_ = mem;
    }
  }
  ++pos;
  return head.next_;
}

static Type *parse_struct_declaration(const TokenList &tokens, u64 &pos) {
  skip_until(tokens, "{", pos);

  Type *ty = new Type(Types::Struct, 0);
  auto members = struct_members(tokens, pos);
  ty->align_ = 1;

  i32 offset = 0;
  for (Member *mem = members; mem; mem = mem->next_) {
    offset = codegen::align_to(offset, mem->type->align_);
    mem->offset = offset;
    offset += mem->type->size_;

    if (ty->align_ < mem->type->align_)
      ty->align_ = mem->type->align_;
  }

  ty->optional_data_ = members;
  ty->size_ = codegen::align_to(offset, ty->align_);
  return ty;
}

static Member *get_struct_member(Type *ty, const token::Token &tok) {
  try {
    Member *ptr = std::get<Member *>(ty->optional_data_);
    for (Member *mem = ptr; mem; mem = ptr->next_) {
      if (strlen(mem->name) == tok.len_ &&
          !strncmp(mem->name, tok.loc_, tok.len_)) {
        return mem;
      }
    }
  } catch (const std::bad_variant_access &e) {
    error("no such struct member.");
  }
  error("no such struct member.");
  std::exit(1);
}

static NodePtr struct_ref(NodePtr lhs, const token::Token &tok) {
  typesystem::add_type(*lhs);
  if (lhs->tt_->type_ != Types::Struct) {
    error("not a struct.");
  }

  auto node = new_single(NodeType::Member, std::move(lhs));
  node->data_ = get_struct_member(node->lhs_->tt_, tok);

  return node;
}

static Type *function_parameters(const TokenList &tokens, u64 &pos,
                                 Type *func_type) {
  std::vector<Type *> params;
  while (tokens[pos] != ")") {
    if (params.size() != 0) {
      skip_until(tokens, ",", pos);
    }

    Type *base = decl_type(tokens, pos);
    Type *tt = declarator(tokens, pos, base);

    params.push_back(tt);
  }

  FunctionType f_data;
  f_data.params_ = std::move(params);
  f_data.return_type_ = func_type;

  Type *func_wrapper = new Type(Types::Function, 0);
  func_wrapper->name_ = strndup(func_type->name_, strlen(func_type->name_));
  func_wrapper->optional_data_ = f_data;

  ++pos;

  return func_wrapper;
}

static Type *type_suffix(const TokenList &tokens, u64 &pos, Type *ty) {
  if (tokens[pos] == "(") {
    ++pos;
    return function_parameters(tokens, pos, ty);
  }

  if (tokens[pos] == "[") {
    ++pos;
    auto sz = get_number_value(tokens, pos);
    skip_until(tokens, "]", pos);
    ty = type_suffix(tokens, pos, ty);

    return typesystem::array_of_type(ty, sz);
  }

  return ty;
}

static Type *declarator(const TokenList &tokens, u64 &pos, Type *ty) {
  while (consume(tokens, pos, "*")) {
    ty = typesystem::ptr_to(ty);
  }

  if (tokens[pos].type_ != token::TokenType::Identifier) {
    error("expected a variable name");
  }

  ty->name_ = strndup(tokens[pos].loc_, tokens[pos].len_);
  ++pos;
  ty = type_suffix(tokens, pos, ty);
  return ty;
}

static NodePtr parse_declaration(const TokenList &tokens, u64 &pos) {
  Type *base = decl_type(tokens, pos);
  int i = 0;
  std::vector<NodePtr> nodes;

  while (tokens[pos] != ";") {
    if (i++ > 0)
      skip_until(tokens, ",", pos);
    Type *ty = declarator(tokens, pos, base);
    auto obj = new_lvar(ty->name_, ty);

    if (tokens[pos] != "=")
      continue;

    auto lhs = new_variable_node(obj);
    ++pos;
    auto rhs = parse_assign(tokens, pos);
    auto node =
        new_binary_node(NodeType::Assign, std::move(lhs), std::move(rhs));
    nodes.push_back(new_single(NodeType::ExprStmt, std::move(node)));
  }

  auto node = new_node(NodeType::Block);
  node->data_ = std::move(nodes);
  ++pos;

  return node;
}

static void create_parameter_lvalues(std::vector<Type *> &params) {
  for (auto &t : params) {
    new_lvar(t->name_, t);
  }
}

static NodePtr parse_stmt(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == "return") {
    ++pos;
    auto node = new_single(NodeType::Return, parse_expression(tokens, pos));
    skip_until(tokens, ";", pos);
    return node;
  }

  if (tokens[pos] == "if") {
    auto node = new_node(NodeType::If);
    skip_until(tokens, "(", pos);

    IfNode if_node;
    if_node.condition_ = parse_expression(tokens, pos);
    skip_until(tokens, ")", pos);
    if_node.then_ = parse_stmt(tokens, pos);
    if (tokens[pos] == "else") {
      ++pos;
      if_node.else_ = parse_stmt(tokens, pos);
    }
    node->data_ = std::move(if_node);

    return node;
  }

  if (tokens[pos] == "for") {
    auto node = new_node(NodeType::For);
    skip_until(tokens, "(", pos);

    ForNode for_node{};
    for_node.initialization_ = parse_expr_stmt(tokens, pos);

    if (tokens[pos] != ";") {
      for_node.condition_ = parse_expression(tokens, pos);
    }
    skip_until(tokens, ";", pos);

    if (tokens[pos] != ")") {
      for_node.increment_ = parse_expression(tokens, pos);
    }
    skip_until(tokens, ")", pos);

    for_node.body_ = parse_stmt(tokens, pos);
    node->data_ = std::move(for_node);
    return node;
  }

  if (tokens[pos] == "while") {
    auto node = new_node(NodeType::For);
    skip_until(tokens, "(", pos);
    ForNode for_node{};
    for_node.condition_ = parse_expression(tokens, pos);
    skip_until(tokens, ")", pos);
    for_node.body_ = parse_stmt(tokens, pos);

    node->data_ = std::move(for_node);

    return node;
  }

  if (tokens[pos] == "{") {
    ++pos;
    return parse_compound_stmt(tokens, pos);
  }

  return parse_expr_stmt(tokens, pos);
}

static NodePtr parse_expression(const TokenList &tokens, u64 &pos) {
  auto node = parse_assign(tokens, pos);

  if (tokens[pos] == ",") {
    ++pos;
    return new_binary_node(NodeType::Comma, std::move(node),
                           parse_expression(tokens, pos));
  }
  return node;
}

static NodePtr parse_equal(const TokenList &tokens, u64 &pos) {
  auto node = parse_relational(tokens, pos);
  for (;;) {
    if (tokens[pos] == "==") {
      ++pos;
      node = new_binary_node(NodeType::EQ, std::move(node),
                             parse_relational(tokens, pos));
      continue;
    }

    if (tokens[pos] == "!=") {
      ++pos;
      node = new_binary_node(NodeType::NE, std::move(node),
                             parse_relational(tokens, pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_postfix(const TokenList &tokens, u64 &pos) {
  auto node = parse_primary(tokens, pos);

  while (tokens[pos] == "[") {
    ++pos;
    auto index = parse_expression(tokens, pos);

    skip_until(tokens, "]", pos);
    node = new_single(NodeType::Derefence,
                      new_addition(std::move(node), std::move(index)));
  }

  return node;
}

static NodePtr parse_assign(const TokenList &tokens, u64 &pos) {
  auto node = parse_equal(tokens, pos);
  if (tokens[pos] == "=") {
    ++pos;
    node = new_binary_node(NodeType::Assign, std::move(node),
                           parse_assign(tokens, pos));
  }

  return node;
}

static NodePtr parse_relational(const TokenList &tokens, u64 &pos) {
  auto node = parse_add(tokens, pos);
  for (;;) {
    if (tokens[pos] == "<") {
      ++pos;
      node = new_binary_node(NodeType::LT, std::move(node),
                             parse_add(tokens, pos));
      continue;
    }

    if (tokens[pos] == "<=") {
      ++pos;
      node = new_binary_node(NodeType::LE, std::move(node),
                             parse_add(tokens, pos));
      continue;
    }

    if (tokens[pos] == ">") {
      ++pos;
      node = new_binary_node(NodeType::LT, parse_add(tokens, pos),
                             std::move(node));
      continue;
    }

    if (tokens[pos] == ">=") {
      ++pos;
      node = new_binary_node(NodeType::LE, parse_add(tokens, pos),
                             std::move(node));
      continue;
    }

    return node;
  }
}

static NodePtr parse_add(const TokenList &tokens, u64 &pos) {
  auto node = parse_mul(tokens, pos);
  for (;;) {
    if (tokens[pos] == "+") {
      ++pos;
      node = new_addition(std::move(node), parse_mul(tokens, pos));
      continue;
    }

    if (tokens[pos] == "-") {
      ++pos;
      node = new_subtraction(std::move(node), parse_mul(tokens, pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_mul(const TokenList &tokens, u64 &pos) {
  auto node = parse_unary(tokens, pos);
  for (;;) {
    if (tokens[pos] == "*") {
      ++pos;
      node = new_binary_node(NodeType::Mul, std::move(node),
                             parse_unary(tokens, pos));
      continue;
    }

    if (tokens[pos] == "/") {
      ++pos;
      node = new_binary_node(NodeType::Div, std::move(node),
                             parse_unary(tokens, pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_unary(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == "+") {
    ++pos;
    return parse_unary(tokens, pos);
  }

  if (tokens[pos] == "-") {
    ++pos;
    return new_single(NodeType::Neg, parse_unary(tokens, pos));
  }

  if (tokens[pos] == "&") {
    ++pos;
    return new_single(NodeType::Addr, parse_unary(tokens, pos));
  }

  if (tokens[pos] == "*") {
    ++pos;
    return new_single(NodeType::Derefence, parse_unary(tokens, pos));
  }

  return parse_postfix(tokens, pos);
}

static NodePtr parse_func_call(const TokenList &tokens, u64 &pos) {
  u64 start_pos = pos;
  skip_until(tokens, "(", pos);

  NodeList nodes;

  while (tokens[pos] != ")") {
    if (nodes.size() != 0) {
      skip_until(tokens, ",", pos);
    }

    nodes.push_back(std::move(parse_assign(tokens, pos)));
  }

  skip_until(tokens, ")", pos);
  auto node = new_node(NodeType::FunctionCall);
  node->func_name_ = strndup(tokens[start_pos].loc_, tokens[start_pos].len_);
  node->data_ = std::move(nodes);

  return node;
}

static NodePtr parse_primary(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == "(" && tokens[pos + 1] == "{") {
    auto node = new_node(NodeType::StmtExpr);
    pos += 2;
    node->data_ = parse_compound_stmt(tokens, pos);
    skip_until(tokens, ")", pos);

    return node;
  }

  if (tokens[pos] == "(") {
    ++pos;
    auto node = parse_expression(tokens, pos);
    skip_until(tokens, ")", pos);

    return node;
  }

  if (tokens[pos] == "sizeof") {
    ++pos;
    auto node = parse_unary(tokens, pos);
    typesystem::add_type(*node);

    return new_number(node->tt_->size_);
  }

  if (tokens[pos].type_ == token::TokenType::Identifier) {
    if (tokens[pos + 1] == "(") {
      return parse_func_call(tokens, pos);
    }

    const auto &token = tokens[pos];
    ++pos;
    auto obj = find_var(token);
    if (obj == nullptr) {
      error("undefined variable");
    }

    return new_variable_node(std::move(obj));
  }

  if (tokens[pos].type_ == token::TokenType::String) {
    const auto &string_literal =
        std::get<token::StringLiteral>(tokens[pos].data_);
    auto len = string_literal.length;
    auto obj = new_string_literal(
        strndup(string_literal.data, len),
        typesystem::array_of_type(new Type(Types::Char, kCharSize),
                                  string_literal.length));
    ++pos;
    return new_variable_node(std::move(obj));
  }

  if (tokens[pos].type_ == token::TokenType::Num) {
    auto node = new_number(std::get<i64>(tokens[pos].data_));
    ++pos;
    return node;
  }

  error("expected primary expression.");
  return nullptr;
}

static NodePtr parse_compound_stmt(const TokenList &tokens, u64 &pos) {
  std::vector<NodePtr> nodes;
  enter_scope();

  while (tokens[pos] != "}") {
    if (is_typename(tokens[pos])) {
      nodes.push_back(std::move(parse_declaration(tokens, pos)));
    } else {
      nodes.push_back(std::move(parse_stmt(tokens, pos)));
    }
    typesystem::add_type(*nodes[nodes.size() - 1]);
  }

  leave_scope();

  auto node = new_node(NodeType::Block);
  node->data_ = std::move(nodes);
  ++pos;

  return node;
}

static void parse_function(const TokenList &tokens, u64 &pos, Type *ty) {
  ty = declarator(tokens, pos, ty);

  try {
    create_parameter_lvalues(
        std::get<FunctionType>(ty->optional_data_).params_);
  } catch (const std::bad_variant_access &e) {
    // no params do nothing.
  }

  enter_scope();
  std::shared_ptr<Object> func_obj =
      new_gvar(strndup(ty->name_, strlen(ty->name_)), ty);
  func_obj->is_func_ = true;

  func_obj->params_ = ObjectList{};
  for (auto &p : locals_) {
    func_obj->params_.push_back(p);
  }

  skip_until(tokens, "{", pos);
  func_obj->body = parse_compound_stmt(tokens, pos);
  func_obj->locals_ = std::move(locals_);

  leave_scope();

  locals_.clear();
}

static void global_varialble(const TokenList &tokens, u64 &pos, Type *base) {
  bool first = true;
  while (!consume(tokens, pos, ";")) {
    if (!first) {
      skip_until(tokens, ",", pos);
    }

    first = false;
    Type *ty = declarator(tokens, pos, base);
    new_gvar(strndup(ty->name_, strlen(ty->name_)), ty);
  }
}

static bool is_func(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == ";") {
    return false;
  }

  u64 original = pos;
  Type *ty = declarator(tokens, pos, default_empty);
  bool verdict = ty->type_ == Types::Function;
  pos = original;

  return verdict;
}

std::vector<std::shared_ptr<Object>> parse_tokens(const TokenList &tokens) {
  u64 pos = 0;

  while (tokens[pos].type_ != token::TokenType::Eof) {
    Type *base_type = decl_type(tokens, pos);

    if (is_func(tokens, pos)) {
      parse_function(tokens, pos, base_type);
      continue;
    }

    global_varialble(tokens, pos, base_type);
  }

  return std::move(globals_);
}
} // namespace parser
