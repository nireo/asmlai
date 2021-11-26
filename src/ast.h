#ifndef LAI_AST_H
#define LAI_AST_H

#include "token.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class AstType {
  Program,
  Identifier,
  ReturnStatement,
  ExpressionStatement,
  CallExpression,
  StringLiteral,
  PrefixExpression,
  InfixExpression,
  BlockStatement,
  IfExpression,
  IntegerLiteral,
  BooleanExpression,
  FunctionLiteral,
  WhileStatement,
  ForStatement,
  TypeChangeAction,
  GlobalStatement,
  Dereference,
  Addr,
  IdentifierAction,
  VarDecl
};

enum class TypeChange {
  Widen,
  Scale,
};

enum ValueT {
  TYPE_VOID,
  TYPE_CHAR,
  TYPE_INT,
  TYPE_LONG,
  TYPE_PTR_VOID,
  TYPE_PTR_CHAR,
  TYPE_PTR_INT,
  TYPE_PTR_LONG,
};

// due to bad design these need to be defined here again.
static ValueT convert_to_ptr(const ValueT type) {
  switch (type) {
  case TYPE_VOID: {
    return TYPE_PTR_VOID;
  }
  case TYPE_CHAR: {
    return TYPE_PTR_CHAR;
  }
  case TYPE_INT: {
    return TYPE_PTR_INT;
  }
  case TYPE_LONG: {
    return TYPE_PTR_LONG;
  }
  default:
    std::fprintf(stderr, "cannot convert type: %d into pointer\n",
                 static_cast<int>(type));
    std::exit(1);
  }
}

static ValueT convert_from_ptr(const ValueT type) {
  switch (type) {
  case TYPE_PTR_VOID:
    return TYPE_VOID;
  case TYPE_PTR_CHAR:
    return TYPE_CHAR;
  case TYPE_PTR_INT:
    return TYPE_INT;
  case TYPE_PTR_LONG:
    return TYPE_LONG;
  default: {
    std::fprintf(stderr, "type is not convertable to normal type.");
    std::exit(1);
  }
  }
}

class Node {
public:
  [[nodiscard]] virtual AstType Type() const noexcept = 0;

  // we need to know the value type such that we can compare if types are
  // applicable.
  [[nodiscard]] virtual ValueT ValueType() const noexcept = 0;
};

class Statement : public Node {
public:
  virtual void statementNode() = 0;
  [[nodiscard]] AstType Type() const noexcept override = 0;
  [[nodiscard]] ValueT ValueType() const noexcept override = 0;
};

class Expression : public Node {
public:
  virtual void set_rvalue(bool) = 0;
  virtual bool is_rvalue() = 0;
  [[nodiscard]] AstType Type() const noexcept override = 0;
  [[nodiscard]] ValueT ValueType() const noexcept override = 0;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

class Program : public Node {
public:
  [[nodiscard]] AstType Type() const noexcept override { return AstType::Program; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::vector<std::unique_ptr<Statement>> statements_;
};

class Identifier : public Expression {
public:
  void set_rvalue(bool value) override { rvalue = value; }
  bool is_rvalue() override { return rvalue; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::Identifier; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return value_type; }

  std::string value_;
  ValueT value_type;
  bool rvalue = false;
};

class ReturnStatement : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::ReturnStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::unique_ptr<Expression> return_value_;
  std::string function_identifier_;
  ValueT types_;
};

class ExpressionStatement : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::ExpressionStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::unique_ptr<Expression> expression_;
};

class IntegerLiteral : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::IntegerLiteral; }

  [[nodiscard]] ValueT ValueType() const noexcept override {
    if (value_ <= 255 && value_ >= 0) {
      return TYPE_CHAR;
    }

    if (value_ > static_cast<std::int64_t>((2147483647)))
      return TYPE_LONG;

    return TYPE_INT;
  }

  std::int64_t value_{};
  bool rvalue = false;
};

class PrefixExpression : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::PrefixExpression; }

  [[nodiscard]] ValueT ValueType() const noexcept override {
    if (opr_ == TokenType::Amper) {
      return TYPE_PTR_INT;
    } else if (opr_ == TokenType::Asterisk) {
      return TYPE_INT;
    }

    return right_->ValueType();
  }

  bool rvalue = false;
  TokenType opr_;
  std::unique_ptr<Expression> right_;
};

class InfixExpression : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::InfixExpression; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return v_type_; }

  TokenType opr;
  std::unique_ptr<Expression> right_;
  std::unique_ptr<Expression> left_;
  ValueT v_type_ = TYPE_VOID;
  bool rvalue = false;
};

class BlockStatement : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::BlockStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::vector<std::unique_ptr<Statement>> statements_;
};

class IfExpression : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::IfExpression; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> after_;
  std::unique_ptr<BlockStatement> other_;
  bool rvalue = false;
};

class WhileStatement : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::WhileStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> body_;
  bool rvalue = false;
};

class FunctionLiteral : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::FunctionLiteral; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::vector<std::unique_ptr<Identifier>> params_;
  std::unique_ptr<BlockStatement> body_;
  std::unique_ptr<Identifier> name_;
  ValueT return_type_;
  bool is_prototype = false;
};

class CallExpression : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::CallExpression; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return func_->ValueType(); }

  std::vector<std::unique_ptr<Expression>> arguments_;
  std::unique_ptr<Expression> func_;
  bool rvalue = false;
};

class StringLiteral : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::StringLiteral; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_PTR_CHAR; }

  bool rvalue = false;
  int id_{};
};

class ForStatement : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::ForStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return TYPE_VOID; }

  std::unique_ptr<Statement> assignment_;
  std::unique_ptr<Expression> cond_;
  std::unique_ptr<Statement> after_every_;
  std::unique_ptr<BlockStatement> body_;
  bool rvalue = false;
};

class TypeChangeAction : public Expression {
public:
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return inner_->ValueType(); }
  [[nodiscard]] AstType Type() const noexcept override { return AstType::TypeChangeAction; }

  TypeChange action_ = TypeChange::Widen;
  std::unique_ptr<Expression> inner_ = nullptr;
  int size = 0;
  bool rvalue = false;
};

class GlobalVariable : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::GlobalStatement; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return type_; }

  ValueT type_;
  std::unique_ptr<Expression> identifier_;
};

class VarDecl : public Statement {
public:
  void statementNode() override {}
  [[nodiscard]] AstType Type() const noexcept override { return AstType::VarDecl; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return type_; }

  ValueT type_;
  std::unique_ptr<Expression> identifier_;
};

class Dereference : public Expression {
public:
  [[nodiscard]] AstType Type() const noexcept override { return AstType::Dereference; }
  [[nodiscard]] ValueT ValueType() const noexcept override {
    return convert_from_ptr(to_dereference_->ValueType());
  }
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }

  std::unique_ptr<Expression> to_dereference_;
  bool rvalue = false;
};

class Addr : public Expression {
public:
  [[nodiscard]] AstType Type() const noexcept override { return AstType::Addr; }
  [[nodiscard]] ValueT ValueType() const noexcept override {
    return convert_to_ptr(to_addr_->ValueType());
  }
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }

  std::unique_ptr<Expression> to_addr_;
  bool rvalue = false;
};

class IdentifierAction : public Expression {
public:
  [[nodiscard]] AstType Type() const noexcept override { return AstType::IdentifierAction; }
  [[nodiscard]] ValueT ValueType() const noexcept override { return identifier_->ValueType(); }
  bool is_rvalue() override { return rvalue; }
  void set_rvalue(bool value) override { rvalue = value; }

  TokenType action_;
  std::unique_ptr<Expression> identifier_;
  bool rvalue = false;
  bool post_ = false;
};

#endif
