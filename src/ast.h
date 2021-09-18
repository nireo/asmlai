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
  HashLiteral,
  ArrayLiteral,
  CallExpression,
  StringLiteral,
  IndexExpression,
  PrefixExpression,
  InfixExpression,
  BlockStatement,
  IfExpression,
  IntegerLiteral,
  BooleanExpression,
  FunctionLiteral,
  PrintStatement,
  WhileStatement,
  ForStatement,
};

class Node
{
public:
  virtual AstType Type() const noexcept = 0;
};

class Statement : public Node
{
public:
  virtual void statementNode() = 0;
  virtual AstType Type() const noexcept = 0;
};

class Expression : public Node
{
public:
  virtual AstType Type() const noexcept = 0;
};

class Program : public Node
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::Program;
  }

  std::vector<std::unique_ptr<Statement> > statements_;
};

class Identifier : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::Identifier;
  }

  std::string value_;
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

  tokentypes type;
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

  std::unique_ptr<Expression> return_value_;
};

class PrintStatement : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::PrintStatement;
  }

  std::unique_ptr<Expression> print_value_;
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

  std::unique_ptr<Expression> expression_;
};

class IntegerLiteral : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::IntegerLiteral;
  }

  std::int64_t value_;
};

class PrefixExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::PrefixExpression;
  }

  tokentypes opr;
  std::unique_ptr<Expression> right_;
};

class InfixExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::InfixExpression;
  }

  tokentypes opr;
  std::unique_ptr<Expression> right_;
  std::unique_ptr<Expression> left_;
};

class BooleanExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::BooleanExpression;
  }

  Token token_;
  bool value_;
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

  Token token;
  std::vector<std::unique_ptr<Statement> > statements_;
};

class IfExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::IfExpression;
  }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> after_;
  std::unique_ptr<BlockStatement> other_;
};

class WhileStatement : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::WhileStatement;
  }

  std::unique_ptr<Expression> cond_;
  std::unique_ptr<BlockStatement> body_;
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

  std::vector<std::unique_ptr<Identifier> > params_;
  std::unique_ptr<BlockStatement> body_;
  std::unique_ptr<Identifier> name_;
};

class CallExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::CallExpression;
  }

  std::vector<std::unique_ptr<Expression> > arguments_;
  std::unique_ptr<Expression> func_;
};

class StringLiteral : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::StringLiteral;
  }

  std::string value_;
};

class ArrayLiteral : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::ArrayLiteral;
  }
  std::vector<std::unique_ptr<Expression> > elements_;
};

class IndexExpression : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::IndexExpression;
  }

  std::unique_ptr<Expression> left_;
  std::unique_ptr<Expression> index_;
};

class ForStatement : public Expression
{
public:
  AstType
  Type() const noexcept
  {
    return AstType::ForStatement;
  }

  std::unique_ptr<Statement> assignment_;
  std::unique_ptr<Expression> cond_;
  std::unique_ptr<Statement> after_every_;
  std::unique_ptr<BlockStatement> body_;
};

#endif
