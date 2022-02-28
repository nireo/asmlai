#ifndef _ASMLAI_PARSER_H
#define _ASMLAI_PARSER_H

#include "token.h"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace parser {

enum class Types {
  Empty,
  Int,
  Ptr,
  Function,
  Array,
};
struct Type;
using TypePtr = std::unique_ptr<Type>;

struct ArrayType {
  i32 array_length;
};

struct FunctionType {
  TypePtr return_type_;
  std::vector<TypePtr> params_;
};

struct Type_ptr : public std::unique_ptr<Type, std::default_delete<Type>> {
  using unique_ptr::unique_ptr;

  Type_ptr(const Type_ptr &o) : unique_ptr<Type, std::default_delete<Type>>() {
    reset(o ? o->clone() : nullptr);
  }
  Type_ptr &operator=(const Type_ptr &o) {
    reset(o ? o->clone() : nullptr);
    return *this;
  }
};

struct Type {
public:
  Type(Types tt) : type_(tt) {}

  i32 size_;
  Types type_;
  Type_ptr base_type_ = nullptr;
  char *name_ = nullptr;
  std::variant<std::vector<Type_ptr>, std::monostate, Type_ptr> optional_data_;
};

extern parser::Type *default_int;
extern parser::Type *default_empty;

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
  FunctionCall,
};

struct Node;
struct Object;

using NodePtr = std::unique_ptr<Node>;
using NodeList = std::vector<NodePtr>;
using ObjectList = std::vector<std::shared_ptr<Object>>;

struct Object {
  Object(char *name, i64 offset) : name_(name), offset_(offset) {}
  char *name_;
  i64 offset_;
  Type *ty_;
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
  Node() : tt_(default_empty) {}
  NodeType type_ = NodeType::Add; // default type
  std::unique_ptr<Node> lhs_ = nullptr;
  std::unique_ptr<Node> rhs_ = nullptr;
  Type *tt_ = NULL;

  std::variant<i64, std::shared_ptr<Object>, NodeList, IfNode, ForNode, char *,
               std::monostate>
      data_ = std::monostate{};

  // This would be normally wrapped into the std::variant, but when calling
  // functions, we also need the arguments list which is stored in the
  // std::variant so we need the new struct member. NOTE: This is used when
  // defining functions and calling them.
  char *func_name_ = NULL;
};

struct Function {
  i64 stack_sz_;
  NodePtr body_;
  ObjectList locals_;
  ObjectList params_;

  char *name_;
};

std::vector<Function> parse_tokens(const std::vector<token::Token> &tokens);
} // namespace parser

#endif
