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
  AssingmentStatement,
  TypeChangeAction,
  GlobalStatement,
  Dereference,
  Addr,
  IdentifierAction,
  VarDecl
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

class Node {
public:
  virtual AstType Type() const noexcept = 0;

  // we need to know the value type such that we can compare if types are
  // applicable.
  virtual ValueT ValueType() const noexcept = 0;
};

class Statement : public Node {
public:
  virtual void statementNode() = 0;
  virtual AstType Type() const noexcept = 0;
  virtual ValueT ValueType() const noexcept = 0;
};

class Expression : public Node {
public:
  virtual void set_rvalue(bool) = 0;
  virtual bool is_rvalue() = 0;
  virtual AstType Type() const noexcept = 0;
  virtual ValueT ValueType() const noexcept = 0;
};

class Program : public Node {
public:
  AstType Type() const noexcept { return AstType::Program; }
  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::vector<std::unique_ptr<Statement>> statements_;
};

class AssignmentStatement : public Statement {
public:
  void statementNode() {}
  AstType Type() const noexcept { return AstType::AssingmentStatement; }

  ValueT ValueType() const noexcept { return assingment_type_; }

  std::unique_ptr<Expression> identifier_;
  std::unique_ptr<Expression> value_;
  ValueT assingment_type_;
};

class Identifier : public Expression {
public:
  void set_rvalue(bool value) { rvalue = value; }

  bool is_rvalue() { return rvalue; }

  AstType Type() const noexcept { return AstType::Identifier; }

  ValueT ValueType() const noexcept { return value_type; }

  std::string value_;
  ValueT value_type;
  bool rvalue = false;
};

class ReturnStatement : public Statement {
public:
  void statementNode() {}

  AstType Type() const noexcept { return AstType::ReturnStatement; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::unique_ptr<Expression> return_value_;
  std::string function_identifier_;
  ValueT types_;
};

class ExpressionStatement : public Statement {
public:
  void statementNode() {}

  AstType Type() const noexcept { return AstType::ExpressionStatement; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::unique_ptr<Expression> expression_;
};

class IntegerLiteral : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::IntegerLiteral; }

  ValueT ValueType() const noexcept { return TYPE_INT; }

  std::int64_t value_;
  bool rvalue = false;
};

class PrefixExpression : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::PrefixExpression; }

  ValueT ValueType() const noexcept {
    if (opr == TokenType::Amper) {
      return TYPE_PTR_INT;
    } else if (opr == TokenType::Asterisk) {
      return TYPE_INT;
    }

    return right_->ValueType();
  }

  bool rvalue = false;
  TokenType opr;
  std::unique_ptr<Expression> right_;
};

class InfixExpression : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::InfixExpression; }

  ValueT ValueType() const noexcept { return v_type_; }

  TokenType opr;
  std::unique_ptr<Expression> right_;
  std::unique_ptr<Expression> left_;
  ValueT v_type_ = TYPE_VOID;
  bool rvalue = false;
};

class BooleanExpression : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::BooleanExpression; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  bool value_;
  bool rvalue = false;
};

class BlockStatement : public Statement {
public:
  void statementNode() {}

  AstType Type() const noexcept { return AstType::BlockStatement; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  Token token;
  std::vector<std::unique_ptr<Statement>> statements_;
};

class IfExpression : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::IfExpression; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> after_;
  std::unique_ptr<BlockStatement> other_;
  bool rvalue = false;
};

class WhileStatement : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::WhileStatement; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> body_;
  bool rvalue = false;
};

class FunctionLiteral : public Statement {
public:
  void statementNode() {}

  AstType Type() const noexcept { return AstType::FunctionLiteral; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::vector<std::unique_ptr<Identifier>> params_;
  std::unique_ptr<BlockStatement> body_;
  std::unique_ptr<Identifier> name_;
  ValueT return_type_;
};

class CallExpression : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::CallExpression; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::vector<std::unique_ptr<Expression>> arguments_;
  std::unique_ptr<Expression> func_;
  bool rvalue = false;
};

class StringLiteral : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::StringLiteral; }

  ValueT ValueType() const noexcept { return TYPE_PTR_CHAR; }

  bool rvalue = false;
  std::string value_;
  int id_;
};

class ForStatement : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  AstType Type() const noexcept { return AstType::ForStatement; }

  ValueT ValueType() const noexcept { return TYPE_VOID; }

  std::unique_ptr<Statement> assignment_;
  std::unique_ptr<Expression> cond_;
  std::unique_ptr<Statement> after_every_;
  std::unique_ptr<BlockStatement> body_;
  bool rvalue = false;
};

enum class TypeChange {
  Widen,
  Scale,
};

class TypeChangeAction : public Expression {
public:
  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  ValueT ValueType() const noexcept { return inner_->ValueType(); }

  AstType Type() const noexcept { return AstType::TypeChangeAction; }

  TypeChange action_ = TypeChange::Widen;
  std::unique_ptr<Expression> inner_ = nullptr;
  int size = 0;
  bool rvalue = false;
};

class GlobalVariable : public Statement {
public:
  void statementNode() {}
  AstType Type() const noexcept { return AstType::GlobalStatement; }
  ValueT ValueType() const noexcept { return type_; }

  ValueT type_;
  std::unique_ptr<Expression> identifier_;
};

class VarDecl : public Statement {
public:
  void statementNode() {}
  AstType Type() const noexcept { return AstType::VarDecl; }
  ValueT ValueType() const noexcept { return type_; }

  ValueT type_;
  std::unique_ptr<Expression> identifier_;
};

class Dereference : public Expression {
public:
  AstType Type() const noexcept { return AstType::Dereference; }
  ValueT ValueType() const noexcept { return TYPE_INT; }
  bool is_rvalue() { return rvalue; }
  void set_rvalue(bool value) { rvalue = value; }

  std::unique_ptr<Expression> to_dereference_;
  bool rvalue = false;
};

class Addr : public Expression {
public:
  AstType Type() const noexcept { return AstType::Addr; }
  ValueT ValueType() const noexcept { return TYPE_PTR_INT; }
  bool is_rvalue() { return rvalue; }
  void set_rvalue(bool value) { rvalue = value; }

  std::unique_ptr<Expression> to_addr_;
  bool rvalue = false;
};

class IdentifierAction : public Expression {
public:
  AstType Type() const noexcept { return AstType::IdentifierAction; }

  ValueT ValueType() const noexcept { return identifier_->ValueType(); }

  bool is_rvalue() { return rvalue; }

  void set_rvalue(bool value) { rvalue = value; }

  TokenType action_;
  std::unique_ptr<Expression> identifier_;
  bool rvalue = false;
  bool post_ = false;
};

#endif
