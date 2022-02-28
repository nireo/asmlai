#include "parser.h"
#include "token.h"
#include "typesystem.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

namespace parser {
template <typename... Args> static void error(const char *fmt, Args... args) {
  std::fprintf(stderr, fmt, args...);
  std::fprintf(stderr, "\n");
  std::exit(1);
}

static std::vector<std::shared_ptr<Object>> locals_;
using TokenList = std::vector<token::Token>;

static NodePtr new_node(NodeType type_) {
  auto node = std::make_unique<Node>();
  node->type_ = type_;

  return node;
}

static std::shared_ptr<Object> find_var(const token::Token &tok) {
  for (const auto &obj : locals_) {
    if (!strncmp(tok.loc_, obj->name_, tok.len_)) {
      return obj;
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

// it needs to be a pointer so that we can change the offset stored in the
// object more easily
static std::shared_ptr<Object> new_lvar(char *name, Type *ty) {
  std::shared_ptr<Object> obj = std::make_shared<Object>(name, 0);
  obj->ty_ = ty;
  locals_.push_back(obj);

  return obj;
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

  if (lhs->tt_->type_ == parser::Types::Int &&
      rhs->tt_->type_ == parser::Types::Int) {
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

  rhs = new_binary_node(NodeType::Mul, std::move(rhs), new_number(8));
  return new_binary_node(NodeType::Add, std::move(lhs), std::move(rhs));
}

static NodePtr new_subtraction(NodePtr lhs_, NodePtr rhs_) {
  auto lhs = std::move(lhs_);
  auto rhs = std::move(rhs_);

  typesystem::add_type(*lhs);
  typesystem::add_type(*rhs);

  if (lhs->tt_->type_ == parser::Types::Int &&
      rhs->tt_->type_ == parser::Types::Int) {
    return new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
  }

  if (lhs->tt_->base_type_ != nullptr &&
      rhs->tt_->type_ == parser::Types::Int) {
    rhs = new_binary_node(NodeType::Mul, std::move(rhs), new_number(8));
    typesystem::add_type(*rhs);

    Type *tt = lhs->tt_;
    auto n = new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
    n->tt_ = tt;
    return n;
  }

  if (lhs->tt_->base_type_ != nullptr && rhs->tt_->base_type_ != nullptr) {
    auto n = new_binary_node(NodeType::Sub, std::move(lhs), std::move(rhs));
    n->tt_ = new parser::Type(Types::Int);
    return new_binary_node(NodeType::Div, std::move(n), new_number(8));
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

static Type *decl_type(const TokenList &tokens, u64 &pos) {
  skip_until(tokens, "int", pos);
  return new Type(Types::Int);
}

static Type *type_suffix(const TokenList &tokens, u64 &pos,
                         Type *ty) { // parse func params
  if (tokens[pos] == "(") {
    ++pos;

    std::vector<Type *> parameters;
    while (tokens[pos] != ")") {
      if (parameters.size() != 0) {
        skip_until(tokens, ",", pos);
      }

      Type *base = decl_type(tokens, pos);
      Type *tt = declarator(tokens, pos, base);

      parameters.push_back(tt);
    }

    ty->optional_data_ = std::move(parameters);
    ++pos;
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
  return parse_assign(tokens, pos);
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

  return parse_primary(tokens, pos);
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
  if (tokens[pos] == "(") {
    ++pos;
    auto node = parse_expression(tokens, pos);
    skip_until(tokens, ")", pos);

    return node;
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
  while (tokens[pos] != "}") {
    if (tokens[pos] == "int") {
      nodes.push_back(std::move(parse_declaration(tokens, pos)));
    } else {
      nodes.push_back(std::move(parse_stmt(tokens, pos)));
    }
    typesystem::add_type(*nodes[nodes.size() - 1]);
  }

  auto node = new_node(NodeType::Block);
  node->data_ = std::move(nodes);
  ++pos;

  return node;
}

static Function parse_function(const TokenList &tokens, u64 &pos) {
  Type *ty = decl_type(tokens, pos);
  ty = declarator(tokens, pos, ty);

  try {
    create_parameter_lvalues(std::get<std::vector<Type *>>(ty->optional_data_));
  } catch (const std::bad_variant_access &e) {
    // no params do nothing.
  }

  Function func;
  func.params_ = ObjectList{};
  for (auto &p : locals_) {
    func.params_.push_back(p);
  }

  func.name_ = ty->name_;
  skip_until(tokens, "{", pos);
  func.body_ = parse_compound_stmt(tokens, pos);
  func.locals_ = std::move(locals_);

  locals_.clear();

  return func;
}

std::vector<Function> parse_tokens(const TokenList &tokens) {
  u64 pos = 0;

  std::vector<Function> funcs;
  while (tokens[pos].type_ != token::TokenType::Eof) {
    funcs.push_back(std::move(parse_function(tokens, pos)));
  }

  return funcs;
}
} // namespace parser
