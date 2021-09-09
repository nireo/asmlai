#ifndef LAI_AST_H
#define LAI_AST_H

#include "lexer.h"
#include <cstdint>
#include <memory>
#include <string>
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
};

class Node {
public:
  virtual AstType Type() const = 0;
};

class Statement : public Node {
public:
  virtual void statementNode() = 0;
  virtual AstType Type() const = 0;
};

class Expression : public Node {
public:
  virtual AstType Type() const = 0;
};

class Program : public Node {
public:
  AstType Type() const { return AstType::Program; }

  std::vector<std::unique_ptr<Statement>> statements;
};

class Identifier : public Expression {
public:
  AstType Type() const { return AstType::Identifier; }
  std::string value;
};

class VarDecl : public Statement {
public:
  void statementNode() {}
  AstType Type() const { return AstType::LetStatement; }

  TokenType type;
  std::unique_ptr<Identifier> name;
  std::unique_ptr<Expression> value;
};

class PrintStmt : public Statement {
public:
  void statementNode() {}
  AstType Type() const { return AstType::LetStatement; }

  TokenType type;
  std::unique_ptr<Expression> value;
};

class ExpressionStatement : public Statement {
public:
  void statementNode() {}
  AstType Type() const { return AstType::ExpressionStatement; }
  std::unique_ptr<Expression> expression;
};

class IntegerLiteral : public Expression {
public:
  AstType Type() const { return AstType::IntegerLiteral; }
  std::int64_t value;
};

class PrefixExpression : public Expression {
public:
  AstType Type() const { return AstType::PrefixExpression; }
  TokenType opr;
  std::unique_ptr<Expression> right;
};

class InfixExpression : public Expression {
public:
  AstType Type() const { return AstType::InfixExpression; }

  TokenType opr;
  std::unique_ptr<Expression> right;
  std::unique_ptr<Expression> left;
};


#endif
