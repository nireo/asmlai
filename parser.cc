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
static std::shared_ptr<Object> current_function_ = nullptr;

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

// we cannot return a reference, since it can also be null. So instead return a
// pointer.
static VarScope *find_var(const token::Token &tok) {
  for (Scope *sc = scopes; sc; sc = sc->next_) {
    for (auto &v : sc->variables_) {
      if (tok == v.name_) {
        return &v;
      }
    }
  }

  return nullptr;
}

static Type *find_tag(const token::Token &tok) {
  for (Scope *sc = scopes; sc; sc = sc->next_) {
    for (const auto &t : sc->tags_) {
      if (tok == t.name_)
        return t.ty_;
    }
  }
  return nullptr;
}

static NodePtr new_cast(NodePtr expr, Type *ty) {
  typesystem::add_type(*expr);

  auto node = std::make_unique<Node>();
  node->type_ = NodeType::Cast;
  node->lhs_ = std::move(expr);
  node->tt_ = ty;

  return node;
}

static void push_tag(const token::Token &tok, Type *type) {
  StructTag tag{};
  tag.name_ = strndup(tok.loc_, tok.len_);
  tag.ty_ = type;

  scopes->tags_.push_back(std::move(tag));
}

static bool consume(const TokenList &tokens, u64 &pos, const char *str) {
  if (tokens[pos] == str) {
    ++pos;
    return true;
  }

  return false;
}

// return the index of the variable in the scope, such that we can edit it
// without having to use pointers for VarScopes.
static u64 push_scope(char *name, std::shared_ptr<Object> variable) {
  VarScope vscope;
  vscope.name_ = name;
  vscope.variable_ = variable;
  scopes->variables_.push_back(std::move(vscope));

  return scopes->variables_.size() - 1;
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
  // '\n'; std::cout << "addition on rhs: " <<
  // static_cast<int>(rhs->tt_->type_)
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
static Type *parse_union_declaration(const TokenList &, u64 &);
static Type *struct_union(const TokenList &tokens, u64 &pos);

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

static Type *find_typedef(const token::Token &tok) {
  if (tok.type_ == token::TokenType::Identifier) {
    auto tp = find_var(tok);
    if (tp) {
      return tp->typedef_;
    }
  }

  return nullptr;
}

static bool is_typename(const token::Token &tok) {
  if (tok == "char" || tok == "int" || tok == "struct" || tok == "union" ||
      tok == "long" || tok == "void")
    return true;

  return find_typedef(tok);
}

static Type *parse_union_declaration(const TokenList &tokens, u64 &pos) {
  Type *ty = struct_union(tokens, pos);
  ty->type_ = Types::Union;

  auto members = std::get<Member *>(ty->optional_data_);
  for (Member *mem = members; mem; mem = mem->next_) {
    if (ty->align_ < mem->type->align_) {
      ty->align_ = mem->type->align_;
    }

    if (ty->size_ < mem->type->size_) {
      ty->size_ = mem->type->size_;
    }
  }
  ty->size_ = codegen::align_to(ty->size_, ty->align_);
  return ty;
}

static Type *decl_type(const TokenList &tokens, u64 &pos,
                       VariableAttributes *attr) {
  constexpr i32 VOID = 1 << 0;
  constexpr i32 CHAR = 1 << 2;
  constexpr i32 SHORT = 1 << 4;
  constexpr i32 INT = 1 << 6;
  constexpr i32 LONG = 1 << 8;
  constexpr i32 OTHER = 1 << 10;

  Type *ty = new Type(Types::Int, kNumberSize, kNumberSize);
  int counter = 0;

  while (is_typename(tokens[pos])) {
    if (tokens[pos] == "typedef") {
      if (!attr) {
        error("typedef not allowed here.");
      }
      attr->is_typedef_ = true;
      ++pos;
      continue;
    }

    Type *ty2 = find_typedef(tokens[pos]);
    if (tokens[pos] == "struct" || tokens[pos] == "union" || ty2) {
      if (counter)
        break;

      if (tokens[pos] == "struct") {
        ++pos;
        ty = parse_struct_declaration(tokens, pos);
      } else if (tokens[pos] == "union") {
        ++pos;
        ty = parse_union_declaration(tokens, pos);
      } else {
        ++pos;
        ty = ty2;
      }

      counter += OTHER;
      continue;
    }

    if (tokens[pos] == "void") {
      counter += VOID;
    } else if (tokens[pos] == "char") {
      counter += CHAR;
    } else if (tokens[pos] == "short") {
      counter += SHORT;
    } else if (tokens[pos] == "int") {
      counter += INT;
    } else if (tokens[pos] == "long") {
      counter += LONG;
    } else {
      std::fprintf(stderr, "undefined type.");
      std::exit(1);
    }

    switch (counter) {
    case VOID: {
      ty = default_void;
      break;
    }
    case CHAR: {
      ty = new Type(Types::Char, kCharSize, kCharSize);
      break;
    }
    case SHORT:
    case SHORT + INT: {
      ty = new Type(Types::Short, kShortSize, kShortSize);
      break;
    }
    case INT: {
      ty = new Type(Types::Int, kNumberSize, kNumberSize);
      break;
    }
    case LONG:
    case LONG + LONG:
    case LONG + LONG + INT:
    case LONG + INT: {
      ty = new Type(Types::Long, kLongSize, kLongSize);
      break;
    }
    default: {
      error("invalid type.");
    }
    }

    ++pos;
  }

  return ty;
}

static Member *struct_members(const TokenList &tokens, u64 &pos) {
  Member head{};
  Member *current = &head;

  while (tokens[pos] != "}") {
    Type *base_type = decl_type(tokens, pos, nullptr);
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

static void parse_typedef(const TokenList &tokens, u64 &pos, Type *base) {
  int i = 0;

  while (!consume(tokens, pos, ";")) {
    if (i++ > 0) {
      skip_until(tokens, ";", pos);
    }
    Type *ty = declarator(tokens, pos, base);

    // TODO: do this
  }
  return;
}

static Type *struct_union(const TokenList &tokens, u64 &pos) {
  i32 tag_pos = -1;
  if (tokens[pos].type_ != token::TokenType::Identifier) {
    tag_pos = pos;
    ++pos;
  }

  if (tag_pos != -1 && tokens[pos] != "{") {
    Type *ty = find_tag(tokens[tag_pos]);
    if (!ty) {
      error("unknown struct type");
    }
    return ty;
  }

  Type *ty = new Type(Types::Struct, 0);
  ++pos;
  auto members = struct_members(tokens, pos);
  ty->align_ = 1;
  ty->optional_data_ = members;

  if (tag_pos != -1) {
    push_tag(tokens[tag_pos], ty);
  }

  return ty;
}

static Type *parse_struct_declaration(const TokenList &tokens, u64 &pos) {
  Type *ty = struct_union(tokens, pos);
  ty->type_ = Types::Struct;

  auto members = std::get<Member *>(ty->optional_data_);

  i32 offset = 0;
  for (Member *mem = members; mem; mem = mem->next_) {
    offset = codegen::align_to(offset, mem->type->align_);
    mem->offset = offset;
    offset += mem->type->size_;

    if (ty->align_ < mem->type->align_)
      ty->align_ = mem->type->align_;
  }
  ty->size_ = codegen::align_to(offset, ty->align_);

  return ty;
}

static NodePtr parse_cast(const TokenList &tokens, u64 &pos) {
  if (tokens[pos] == "(" && is_typename(tokens[pos + 1])) {
    u64 original_pos = pos;
    ++pos;
    Type *ty = decl_type(tokens, pos, nullptr);
    skip_until(tokens, ")", pos);
  }

  return parse_unary(tokens, pos);
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
  if (lhs->tt_->type_ != Types::Struct && lhs->tt_->type_ != Types::Union) {
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

    Type *base = decl_type(tokens, pos, nullptr);
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

static NodePtr parse_declaration(const TokenList &tokens, u64 &pos,
                                 Type *base) {
  int i = 0;
  std::vector<NodePtr> nodes;

  while (tokens[pos] != ";") {
    if (i++ > 0)
      skip_until(tokens, ",", pos);
    Type *ty = declarator(tokens, pos, base);
    if (ty->type_ == Types::Void)
      error("variable declared void");

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

  for (;;) {
    if (tokens[pos] == "[") {
      ++pos;
      auto index = parse_expression(tokens, pos);

      skip_until(tokens, "]", pos);
      node = new_single(NodeType::Derefence,
                        new_addition(std::move(node), std::move(index)));
    }

    if (tokens[pos] == ".") {
      ++pos;
      node = struct_ref(std::move(node), tokens[pos]);
      ++pos;
      continue;
    }

    if (tokens[pos] == "->") {
      auto nod = new_single(NodeType::Derefence, std::move(node));
      nod = struct_ref(std::move(nod), tokens[pos + 1]);

      pos += 2;
      continue;
    }
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
    if (!obj || !obj->variable_) {
      error("undefined variable");
    }

    return new_variable_node(obj->variable_);
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
      VariableAttributes attrs{};
      Type *baset = decl_type(tokens, pos, &attrs);

      if (attrs.is_typedef_) {
        parse_typedef(tokens, pos, baset);
        continue;
      }

      nodes.push_back(std::move(parse_declaration(tokens, pos, baset)));
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
  func_obj->is_definition_ = !consume(tokens, pos, ";");

  if (!func_obj->is_definition_) {
    // function prototype.
    return;
  }

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
    VariableAttributes attrs{};
    Type *base_type = decl_type(tokens, pos, &attrs);
    if (attrs.is_typedef_) {
      parse_typedef(tokens, pos, base_type);
      continue;
    }

    if (is_func(tokens, pos)) {
      parse_function(tokens, pos, base_type);
      continue;
    }

    global_varialble(tokens, pos, base_type);
  }

  return std::move(globals_);
}
} // namespace parser
