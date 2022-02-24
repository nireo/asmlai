#ifndef _ASMLAI_PARSER_H
#define _ASMLAI_PARSER_H

#include "token.h"
#include <memory>
#include <variant>
#include <vector>

namespace parser {

enum class Types {
  Int,
  Ptr,
};

struct Type {
  Types type_;
  std::unique_ptr<Type> base_type_ = nullptr;
};

enum class NodeType {
  Add,
  Sub,
  Mul,
  Div,
  Neg,
  EQ,
  NE,
  LT,
  LE,
  Num,
  ExprStmt,
  Assign,
  Variable,
  Return,
  Block,
  If,
  For,
  Addr,
  Derefence,
};

struct Node;
using NodePtr = std::unique_ptr<Node>;

struct Object {
  Object(char *name, i64 offset) : name_(name), offset_(offset) {}
  char *name_;
  i64 offset_;
};

struct IfNode {
  NodePtr condition_ = nullptr;
  NodePtr then_ = nullptr;
  NodePtr else_ = nullptr;
};

struct ForNode {
  NodePtr condition_ = nullptr;
  NodePtr initialization_ = nullptr;
  NodePtr increment_ = nullptr;
  NodePtr body_ = nullptr;
};

struct Node {
  NodeType type_ = NodeType::Add; // default type
  std::unique_ptr<Node> lhs_ = nullptr;
  std::unique_ptr<Node> rhs_ = nullptr;
  std::unique_ptr<Type> tt_ = nullptr;

  std::variant<i64, std::shared_ptr<Object>, std::vector<std::unique_ptr<Node>>,
               IfNode, ForNode, std::monostate>
      data_ = std::monostate{};
};

struct Function {
  i64 stack_sz_;
  std::vector<NodePtr> body_;
  std::vector<std::shared_ptr<Object>> locals_;
};

Function parse_tokens(const std::vector<token::Token> &tokens);
} // namespace parser

#endif
