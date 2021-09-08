#ifndef LAI_AST_H
#define LAI_AST_H

#include "token.h"
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
};

class Node {
public:
  virtual std::string TokenLiteral() const = 0;
  virtual std::string String() = 0;
  virtual AstType Type() const = 0;
};

class Statement : public Node {
public:
  virtual void statementNode() = 0;
  virtual std::string TokenLiteral() const = 0;
  virtual std::string String() = 0;
  virtual AstType Type() const = 0;
};

class Expression : public Node {
public:
  virtual std::string TokenLiteral() const = 0;
  virtual std::string String() = 0;
  virtual AstType Type() const = 0;
};

class Program : public Node {
public:
  std::string TokenLiteral() const;
  std::string String();
  AstType Type() const { return AstType::Program; }

  std::vector<std::unique_ptr<Statement>> statements;
};

class Identifier : public Expression {
public:
  std::string TokenLiteral() const { return token.literal; }
  std::string String() { return value; }
  AstType Type() const { return AstType::Identifier; }

  Token token;
  std::string value;
};

class LetStatement : public Statement {
public:
  void statementNode() {}
  std::string TokenLiteral() const { return token.literal; }
  std::string String();
  AstType Type() const { return AstType::LetStatement; }

  Token token;
  std::unique_ptr<Identifier> name;
  std::unique_ptr<Expression> value;
};

class ReturnStatement : public Statement {
public:
  void statementNode() {}
  std::string TokenLiteral() const { return token.literal; }
  std::string String();
  AstType Type() const { return AstType::ReturnStatement; }

  Token token;
  std::unique_ptr<Expression> return_value;
};

class ExpressionStatement : public Statement {
public:
  void statementNode() {}
  std::string TokenLiteral() const { return token.literal; }
  std::string String();
  AstType Type() const { return AstType::ExpressionStatement; }

  Token token;
  std::unique_ptr<Expression> expression;
};

class IntegerLiteral : public Expression {
public:
  std::string TokenLiteral() const { return token.literal; }
  std::string String() { return token.literal; }
  AstType Type() const { return AstType::IntegerLiteral; }

  Token token;
  int value;
};

class PrefixExpression : public Expression {
public:
  std::string TokenLiteral() const { return token.literal; }
  std::string String();
  AstType Type() const { return AstType::PrefixExpression; }

  Token token;

  // the operator
  std::string opr;
  std::unique_ptr<Expression> right;
};

class InfixExpression : public Expression {
public:
  std::string TokenLiteral() const { return token.literal; }
  std::string String();
  AstType Type() const { return AstType::InfixExpression; }

  Token token;
  std::string opr;
  std::unique_ptr<Expression> right;
  std::unique_ptr<Expression> left;
};

class BooleanExpression : public Expression {
public:
  std::string String() { return token.literal; }
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::BooleanExpression; }

  Token token;
  bool value;
};

class BlockStatement : public Statement {
public:
  void statementNode() {}
  std::string String() { return token.literal; }
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::BlockStatement; }

  Token token;
  std::vector<std::unique_ptr<Statement>> statements;
};

class IfExpression : public Expression {
public:
  std::string String() { return token.literal; }
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::IfExpression; }

  Token token;
  std::unique_ptr<Expression> cond;
  std::unique_ptr<BlockStatement> after;
  std::unique_ptr<BlockStatement> other;
};

class FunctionLiteral : public Expression {
public:
  std::string String() { return token.literal; }
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::FunctionLiteral; }

  Token token;
  std::vector<std::unique_ptr<Identifier>> params;
  std::unique_ptr<BlockStatement> body;
};

class CallExpression : public Expression {
public:
  std::string String();
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::CallExpression; }

  Token token;
  std::vector<std::unique_ptr<Expression>> arguments;
  std::unique_ptr<Expression> func;
};

class StringLiteral : public Expression {
public:
  std::string String() { return value; }
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::StringLiteral; }

  Token token;
  std::string value;
};

class ArrayLiteral : public Expression {
public:
  std::string String();
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::ArrayLiteral; }

  std::vector<std::unique_ptr<Expression>> elements;
  Token token;
};

class IndexExpression : public Expression {
public:
  std::string String();
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::IndexExpression; }

  Token token;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> index;
};

class HashLiteral : public Expression {
public:
  std::string String() { return "hashliteral string"; };
  std::string TokenLiteral() const { return token.literal; }
  AstType Type() const { return AstType::HashLiteral; }

  Token token;

  // we can use maps put there i found some errors for iterating them when it
  // consists only of unique pointers. That is why it is a vector and we don't
  // need to query anything from the ast node so it just works.
  std::vector<
      std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>
      pairs;
};

#endif
