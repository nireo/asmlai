#include "parser.h"
#include "token.h"

namespace parser {
#define CURR tokens[token_pos];

static NodePtr new_node(NodeType type_) {
  auto node = std::make_unique<Node>();
  node->type_ = type_;

  return node;
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

static NodePtr new_number(i64 value) {
  auto node = new_node(NodeType::Num);
  node->data_ = value;

  return node;
}

static void skip_until(const std::vector<token::Token> &tokens, const char *tok,
                       u64 &pos) {
  while (!(tokens[pos] == tok)) {
    ++pos;
  }
  ++pos; // skip the wanted token

  return;
}

static NodePtr parse_expression(const std::vector<token::Token> &, u64 &);
static NodePtr parse_equal(const std::vector<token::Token> &, u64 &);
static NodePtr parse_mul(const std::vector<token::Token> &, u64 &);
static NodePtr parse_add(const std::vector<token::Token> &, u64 &);
static NodePtr parse_unary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_primary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_relational(const std::vector<token::Token> &, u64 &);

static NodePtr parse_expr_stmt(const std::vector<token::Token> &tokens,
                               u64 &pos) {
  auto node = new_single(NodeType::ExprStmt, parse_expression(tokens, pos));
  skip_until(tokens, ";", pos);
  return node;
}

static NodePtr parse_stmt(const std::vector<token::Token> &tokens, u64 &pos) {
  return parse_expr_stmt(tokens, pos);
}

static NodePtr parse_expression(const std::vector<token::Token> &tokens,
                                u64 &pos) {
  return parse_equal(tokens, pos);
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

  if (tokens[pos].type_ == token::TokenType::Num) {
    auto node = new_number(std::get<i64>(tokens[pos].data_));
    ++pos;
    return node;
  }

  return nullptr;
}

std::vector<NodePtr> parse_tokens(const std::vector<token::Token> &tokens) {
  u64 pos = 0;
  std::vector<NodePtr> nodes;
  while (tokens[pos].type_ != token::TokenType::Eof) {
    nodes.push_back(std::move(parse_stmt(tokens, pos)));
  }

  return nodes;
}

} // namespace parser
