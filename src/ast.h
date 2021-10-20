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
  LetStatement,
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
  IdentifierAction
};

enum valuetype {
  TYPE_VOID,
  TYPE_CHAR,
  TYPE_INT,
  TYPE_LONG,
  TYPE_PTR_VOID,
  TYPE_PTR_CHAR,
  TYPE_PTR_INT,
  TYPE_PTR_LONG,
};

class Node
{
public:
  virtual AstType Type() const noexcept = 0;

  // we need to know the value type such that we can compare if types are
  // applicable.
  virtual valuetype ValueType() const noexcept = 0;
};

class Statement : public Node
{
public:
  virtual void statementNode() = 0;
  virtual AstType Type() const noexcept = 0;
  virtual valuetype ValueType() const noexcept = 0;
};

class Expression : public Node
{
public:
  virtual void set_rvalue(bool) = 0;
  virtual bool is_rvalue() = 0;
  virtual AstType Type() const noexcept = 0;
  virtual valuetype ValueType() const noexcept = 0;
};

class Program : public Node
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::Program;
  }
  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::vector<std::unique_ptr<Statement> > statements_;
};

class AssignmentStatement : public Statement
{
public:
  void
  statementNode()
  {
  }
  AstType
  Type() const noexcept
  {
    return AstType::AssingmentStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return assingment_type_;
  }

  std::unique_ptr<Expression> identifier_;
  std::unique_ptr<Expression> value_;
  valuetype assingment_type_;
};

class Identifier : public Expression
{
public:
  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  bool
  is_rvalue()
  {
    return rvalue;
  }

  AstType
  Type() const noexcept
  {
    return AstType::Identifier;
  }

  valuetype
  ValueType() const noexcept
  {
    return value_type;
  }

  std::string value_;
  valuetype value_type;
  bool rvalue = false;
};

class LetStatement : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::LetStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  tokentypes type;
  valuetype v_type;
  std::unique_ptr<Identifier> name_;
  std::unique_ptr<Expression> value_;
};

class ReturnStatement : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::ReturnStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::unique_ptr<Expression> return_value_;
  std::string function_identifier_;
  valuetype types_;
};

class ExpressionStatement : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::ExpressionStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::unique_ptr<Expression> expression_;
};

class IntegerLiteral : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::IntegerLiteral;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_INT;
  }

  std::int64_t value_;
  bool rvalue = false;
};

class PrefixExpression : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::PrefixExpression;
  }

  valuetype
  ValueType() const noexcept
  {
    if(opr == tokentypes::Amper) {
      return TYPE_PTR_INT;
    } else if(opr == tokentypes::Asterisk) {
      return TYPE_INT;
    }

    return right_->ValueType();
  }

  bool rvalue = false;
  tokentypes opr;
  std::unique_ptr<Expression> right_;
};

class InfixExpression : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::InfixExpression;
  }

  valuetype
  ValueType() const noexcept
  {
    return v_type_;
  }

  tokentypes opr;
  std::unique_ptr<Expression> right_;
  std::unique_ptr<Expression> left_;
  valuetype v_type_ = TYPE_VOID;
  bool rvalue = false;
};

class BooleanExpression : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::BooleanExpression;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  bool value_;
  bool rvalue = false;
};

class BlockStatement : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::BlockStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  Token token;
  std::vector<std::unique_ptr<Statement> > statements_;
};

class IfExpression : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::IfExpression;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> after_;
  std::unique_ptr<BlockStatement> other_;
  bool rvalue = false;
};

class WhileStatement : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::WhileStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> body_;
  bool rvalue = false;
};

class FunctionLiteral : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::FunctionLiteral;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::vector<std::unique_ptr<Identifier> > params_;
  std::unique_ptr<BlockStatement> body_;
  std::unique_ptr<Identifier> name_;
  valuetype return_type_;
};

class CallExpression : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::CallExpression;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

  std::vector<std::unique_ptr<Expression> > arguments_;
  std::unique_ptr<Expression> func_;
  bool rvalue = false;
};

class StringLiteral : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::StringLiteral;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_PTR_CHAR;
  }

  bool rvalue = false;
  std::string value_;
  int id_;
};

class ForStatement : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  AstType
  Type() const noexcept
  {
    return AstType::ForStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_VOID;
  }

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

class TypeChangeAction : public Expression
{
public:
  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  valuetype
  ValueType() const noexcept
  {
    return inner_->ValueType();
  }

  AstType
  Type() const noexcept
  {
    return AstType::TypeChangeAction;
  }

  TypeChange action_ = TypeChange::Widen;
  std::unique_ptr<Expression> inner_ = nullptr;
  int size = 0;
  bool rvalue = false;
};

class GlobalVariable : public Statement
{
public:
  void
  statementNode()
  {
  }

  AstType
  Type() const noexcept
  {
    return AstType::GlobalStatement;
  }

  valuetype
  ValueType() const noexcept
  {
    return type_;
  }

  valuetype type_;
  std::unique_ptr<Expression> identifier_;
};

class Dereference : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::Dereference;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_INT;
  }

  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  std::unique_ptr<Expression> to_dereference_;
  bool rvalue = false;
};

class Addr : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::Addr;
  }

  valuetype
  ValueType() const noexcept
  {
    return TYPE_PTR_INT;
  }

  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  std::unique_ptr<Expression> to_addr_;
  bool rvalue = false;
};

class IdentifierAction : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::IdentifierAction;
  }

  valuetype
  ValueType() const noexcept
  {
    return identifier_->ValueType();
  }

  bool
  is_rvalue()
  {
    return rvalue;
  }

  void
  set_rvalue(bool value)
  {
    rvalue = value;
  }

  tokentypes action_;
  std::unique_ptr<Expression> identifier_;
  bool rvalue = false;
  bool post_ = false;
};

#endif
