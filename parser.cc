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

static u64 skip_until(const std::vector<token::Token> &tokens, const char *tok,
                      u64 &pos) {
  while (!(tokens[pos] == tok)) {
    ++pos;
  }
  ++pos; // skip the wanted token
  return pos;
}

static NodePtr parse_expression(const std::vector<token::Token> &, u64 &);
static NodePtr parse_equal(const std::vector<token::Token> &, u64 &);
static NodePtr parse_mul(const std::vector<token::Token> &, u64 &);
static NodePtr parse_add(const std::vector<token::Token> &, u64 &);
static NodePtr parse_unary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_primary(const std::vector<token::Token> &, u64 &);
static NodePtr parse_relational(const std::vector<token::Token> &, u64 &);

static NodePtr parse_expression(const std::vector<token::Token> &tokens,
                                u64 &token_pos) {
  return parse_equal(tokens, token_pos);
}

static NodePtr parse_equal(const std::vector<token::Token> &tokens,
                           u64 &token_pos) {
  auto node = parse_relational(tokens, token_pos);
  for (;;) {
    if (tokens[token_pos] == "==") {
      ++token_pos;
      node = new_binary_node(NodeType::EQ, std::move(node),
                             parse_relational(tokens, token_pos));
      continue;
    }

    if (tokens[token_pos] == "!=") {
      ++token_pos;
      node = new_binary_node(NodeType::NE, std::move(node),
                             parse_relational(tokens, token_pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_relational(const std::vector<token::Token> &tokens,
                                u64 &token_pos) {
  auto node = parse_add(tokens, token_pos);
  for (;;) {
    if (tokens[token_pos] == "<") {
      ++token_pos;
      node = new_binary_node(NodeType::LT, std::move(node),
                             parse_add(tokens, token_pos));
      continue;
    }

    if (tokens[token_pos] == "<=") {
      ++token_pos;
      node = new_binary_node(NodeType::LE, std::move(node),
                             parse_add(tokens, token_pos));
      continue;
    }

    if (tokens[token_pos] == ">") {
      ++token_pos;
      node = new_binary_node(NodeType::LT, parse_add(tokens, token_pos),
                             std::move(node));
      continue;
    }

    if (tokens[token_pos] == ">=") {
      ++token_pos;
      node = new_binary_node(NodeType::LE, parse_add(tokens, token_pos),
                             std::move(node));
      continue;
    }

    return node;
  }
}

static NodePtr parse_add(const std::vector<token::Token> &tokens,
                         u64 &token_pos) {

  auto node = parse_mul(tokens, token_pos);
  for (;;) {
    if (tokens[token_pos] == "+") {
      ++token_pos;
      node = new_binary_node(NodeType::Add, std::move(node),
                             parse_mul(tokens, token_pos));
      continue;
    }

    if (tokens[token_pos] == "-") {
      ++token_pos;
      node = new_binary_node(NodeType::Sub, std::move(node),
                             parse_mul(tokens, token_pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_mul(const std::vector<token::Token> &tokens,
                         u64 &token_pos) {
  auto node = parse_unary(tokens, token_pos);
  for (;;) {
    if (tokens[token_pos] == "*") {
      ++token_pos;
      node = new_binary_node(NodeType::Mul, std::move(node),
                             parse_unary(tokens, token_pos));
      continue;
    }

    if (tokens[token_pos] == "/") {
      ++token_pos;
      node = new_binary_node(NodeType::Div, std::move(node),
                             parse_unary(tokens, token_pos));
      continue;
    }

    return node;
  }
}

static NodePtr parse_unary(const std::vector<token::Token> &tokens,
                           u64 &token_pos) {

  if (tokens[token_pos] == "+") {
    ++token_pos;
    return parse_unary(tokens, token_pos);
  }

  if (tokens[token_pos] == "-") {
    ++token_pos;
    return new_single(NodeType::Neg, parse_unary(tokens, token_pos));
  }

  return parse_primary(tokens, token_pos);
}

static NodePtr parse_primary(const std::vector<token::Token> &tokens,
                             u64 &token_pos) {
  if (tokens[token_pos] == "(") {
    ++token_pos;
    auto node = parse_expression(tokens, token_pos);
    skip_until(tokens, ")", token_pos);

    return node;
  }

  if (tokens[token_pos].type_ == token::TokenType::Num) {
    auto node = new_number(std::get<i64>(tokens[token_pos].data_));
    ++token_pos;
    return node;
  }

  return nullptr;
}

} // namespace parser
