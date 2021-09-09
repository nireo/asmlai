#ifndef LAI_AST_H
#define LAI_AST_H

#include "lexer.h"
#include <cstdint>
#include <memory>
#include <string>

enum class StmtType {
  Empty,
  VarDecl,
  Return,
  Expression,
  Block,
  Print,
};

enum class ExprType {
  Empty,
  Identifier,
  Call,
  StringLiteral,
  Prefix,
  Infix,
  If,
  IntegerLiteral,
  Boolean,
  FunctionLiteral,
};

struct Expression;
struct Statement;

struct IdentifierExpr {
  std::string name_;
};

struct InfixExpr {
  TokenType opr;

  std::unique_ptr<Expression> expr_left_;
  std::unique_ptr<Expression> expr_right_;
};

struct VariableDecl {
  std::unique_ptr<IdentifierExpr> ident_;
  std::unique_ptr<Expression> expr_;
  VariableDecl() : expr_(nullptr){};
};

struct ExprStmt {
  std::unique_ptr<Expression> expr_;
};

struct PrintStmt {
  std::unique_ptr<Expression> expr_;
};

struct Statement {
  StmtType type_;
  union {
    std::uintptr_t is_null_;
    std::unique_ptr<VariableDecl> var_decl_;
    std::unique_ptr<ExprStmt> expr_stmt_;
    std::unique_ptr<PrintStmt> print_stmt_;
  };

  Statement() : type_(StmtType::Empty) {}
  Statement(StmtType type) : type_(type) {}

};

struct Expression {
  ExprType type_;
  union {
    std::uintptr_t is_null_;
    std::int64_t int_lit_;
    std::unique_ptr<InfixExpr> infix;
  };

  Expression() : type_(ExprType::Empty) {}
  Expression(ExprType type) : type_(type) {}
};

#endif
