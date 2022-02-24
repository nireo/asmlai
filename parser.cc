#include "parser.h"
#include "token.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>

namespace parser {
template <typename... Args> static void error(const char *fmt, Args... args) {
  std::fprintf(stderr, fmt, args...);
  std::fprintf(stderr, "\n");
  std::exit(1);
}

static std::vector<std::shared_ptr<Object>> locals_;

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

// it needs to be pointer so that we can change the
static std::shared_ptr<Object> new_lvar(char *name) {
  std::shared_ptr<Object> obj = std::make_shared<Object>(name, 0);
  locals_.push_back(obj);

  return obj;
}

static void skip_until(const std::vector<token::Token> &tokens, const char *tok,
                       u64 &pos) {
  while (!(tokens[pos] == tok)) {
    ++pos;
  }
  ++pos; // skip the wanted token
}

static NodePtr parse_compound_stmt(const std::vector<token::Token> &, u64 &);
static NodePtr parse_expression(const std::vector<token::Token> &, u64 &);
static NodePtr parse_equal(const std::vector<token::Token> &, u64 &);
static NodePtr parse_mul(const std::vector<token::Token> &, u64 &);
static NodePtr parse_add(const std::vector<token::Token> &, u64 &);
static NodePtr parse_unary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_primary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_relational(const std::vector<token::Token> &, u64 &);
static NodePtr parse_assign(const std::vector<token::Token> &, u64 &);

static NodePtr parse_expr_stmt(const std::vector<token::Token> &tokens,
                               u64 &pos) {
  if (tokens[pos] == ";") { // encountered a empty statement
    ++pos;
    return new_node(NodeType::Block);
  }

  auto node = new_single(NodeType::ExprStmt, parse_expression(tokens, pos));
  skip_until(tokens, ";", pos);
  return node;
}

static NodePtr parse_stmt(const std::vector<token::Token> &tokens, u64 &pos) {
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

static NodePtr parse_expression(const std::vector<token::Token> &tokens,
                                u64 &pos) {
  return parse_assign(tokens, pos);
}

static NodePtr parse_equal(const std::vector<token::Token> &tokens, u64 &pos) {
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

static NodePtr parse_assign(const std::vector<token::Token> &tokens, u64 &pos) {
  auto node = parse_equal(tokens, pos);
  if (tokens[pos] == "=") {
    ++pos;
    node = new_binary_node(NodeType::Assign, std::move(node),
                           parse_assign(tokens, pos));
  }

  return node;
}

static NodePtr parse_relational(const std::vector<token::Token> &tokens,
                                u64 &pos) {
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

static NodePtr parse_add(const std::vector<token::Token> &tokens, u64 &pos) {
  auto node = parse_mul(tokens, pos);
  for (;;) {
    if (tokens[pos] == "+") {
      ++pos;
      node = new_binary_node(NodeType::Add, std::move(node),
                             parse_mul(tokens, pos));
      continue;
    }

    if (tokens[pos] == "-") {
      ++pos;
      node = new_binary_node(NodeType::Sub, std::move(node),
                             parse_mul(tokens, pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_mul(const std::vector<token::Token> &tokens, u64 &pos) {
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

static NodePtr parse_unary(const std::vector<token::Token> &tokens, u64 &pos) {
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

static NodePtr parse_primary(const std::vector<token::Token> &tokens,
                             u64 &pos) {
  if (tokens[pos] == "(") {
    ++pos;
    auto node = parse_expression(tokens, pos);
    skip_until(tokens, ")", pos);

    return node;
  }

  if (tokens[pos].type_ == token::TokenType::Identifier) {
    const auto &token = tokens[pos];
    ++pos;
    auto obj = find_var(token);
    if (obj == nullptr) {
      return new_variable_node(new_lvar(strndup(token.loc_, token.len_)));
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

static NodePtr parse_compound_stmt(const std::vector<token::Token> &tokens,
                                   u64 &pos) {
  std::vector<NodePtr> nodes;
  while (tokens[pos] != "}") {
    nodes.push_back(std::move(parse_stmt(tokens, pos)));
  }

  auto node = new_node(NodeType::Block);
  node->data_ = std::move(nodes);
  ++pos;

  return node;
}

Function parse_tokens(const std::vector<token::Token> &tokens) {
  u64 pos = 0;
  std::vector<NodePtr> nodes;
  while (tokens[pos].type_ != token::TokenType::Eof) {
    nodes.push_back(std::move(parse_stmt(tokens, pos)));
  }

  auto func = Function{
      .stack_sz_ = 0,
      .body_ = std::move(nodes),
      .locals_ = std::move(locals_),
  };

  return func;
}
} // namespace parser
