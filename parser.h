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
  Char,
  Int,
  Ptr,
  Function,
  Array,
  Struct,
  Short,
  Union,
  Long,
  Void,
  Bool,
  Enum,
};

constexpr i32 kCharSize = 1;
constexpr i32 kShortSize = 2;
constexpr i32 kNumberSize = 4;
constexpr i32 kLongSize = 8;
constexpr i32 kPtrSize = 8;

struct Type;
using TypePtr = std::unique_ptr<Type>;

struct StructTag {
  char *name_ = nullptr;
  Type *ty_ = nullptr;
};

struct VariableAttributes {
  bool is_typedef_;
};

struct ArrayType {
  i32 array_length;
};

struct FunctionType {
  Type *return_type_;
  std::vector<Type *> params_;
};

struct Member {
  i64 offset = 0;

  char *name;
  Member *next_ = nullptr;
  Type *type = nullptr;
};

using MemberPtr = std::unique_ptr<Member>;
using MemberList = std::vector<Member>;

struct Type {
public:
  Type(Types tt, i32 size) : size_(size), type_(tt) {}
  Type(Types tt, i32 size, i32 align) : type_(tt), size_(size), align_(align) {}

  i32 align_;
  i32 size_ = 0;
  Types type_;
  Type *base_type_ = nullptr;
  char *name_ = nullptr;
  std::variant<std::vector<Type *>, std::monostate, Type *, FunctionType,
               ArrayType, Member *>
      optional_data_;
};

struct TagScope {
  TagScope *next = nullptr;
  char *name = nullptr;
  Type *ty = nullptr;
};

struct Scope;
extern parser::Type *default_int;
extern parser::Type *default_empty;
extern parser::Type *default_long;
extern parser::Type *default_void;
extern Scope
    *scopes; // use linked list since globals vectors didn't work very well.
extern TagScope *tag_scopes;

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
  StmtExpr,
  Comma,
  Member,
  Cast,
  Mod,
  Not,
  LogAnd,
  LogOr,
  BitAnd,
  BitOr,
  BitXor,
  Goto,
  Label,
  Cond,
  Shl,
  Shr
};

struct Node;
struct Object;
struct Function;

using NodePtr = std::unique_ptr<Node>;
using NodeList = std::vector<NodePtr>;
using ObjectList = std::vector<std::shared_ptr<Object>>;

struct LabelGotoData {
  char *label;
  char *unique_label;
  NodePtr goto_;
};

struct Object {
  Object(char *name, i64 offset) : name_(name), offset_(offset) {}
  char *name_ = nullptr;
  i64 offset_ = 0;
  Type *ty_ = nullptr;
  char *init_data_ = nullptr;

  bool is_local_ = false;
  bool is_func_ = false;
  bool is_definition_ = false;
  NodePtr body = nullptr;
  int stack_sz = 0;

  std::vector<std::shared_ptr<Object>> params_{};
  std::vector<std::shared_ptr<Object>> locals_{};
};

struct EnumVarScope {
  Type *enum_type = nullptr;
  int enum_val;
};

struct VarScope {
  char *name_ = nullptr;
  std::shared_ptr<Object> variable_ = nullptr;
  Type *typedef_ = nullptr;
  std::variant<EnumVarScope, std::monostate> data_ =
      std::monostate{}; // we want to optionally store enum data.
};

struct Scope {
  Scope() {
    next_ = nullptr;
    variables_ = std::vector<VarScope>();
    tags_ = std::vector<TagScope>();
  }

  Scope *next_;
  std::vector<VarScope> variables_;
  std::vector<TagScope> tags_;
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
  Type *tt_ = nullptr;

  std::variant<i64, std::shared_ptr<Object>, NodeList, IfNode, ForNode, char *,
               NodePtr, Member *, LabelGotoData, std::monostate>
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

std::vector<std::shared_ptr<Object>>
parse_tokens(const std::vector<token::Token> &tokens);
} // namespace parser

#endif
