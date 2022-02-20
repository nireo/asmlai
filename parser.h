#ifndef _ASMLAI_PARSER_H
#define _ASMLAI_PARSER_H

#include "token.h"
#include <memory>
#include <variant>
#include <vector>

namespace parser {

enum class NodeType { Add, Sub, Mul, Div, Neg, EQ, NE, LT, LE, Num };
struct Node {
  NodeType type_ = NodeType::Add; // default type
  std::unique_ptr<Node> lhs_ = nullptr;
  std::unique_ptr<Node> rhs_ = nullptr;
  std::variant<i64, std::monostate> data_ = std::monostate{};
};

using NodePtr = std::unique_ptr<Node>;
std::vector<NodePtr> parse_tokens(const std::vector<token::Token> &tokens);
} // namespace parser

#endif
