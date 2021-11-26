#ifndef LAI_PARSER_H
#define LAI_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"

#include <memory>

enum Precedence {
  LOWEST,
  SINGLE,
  EQUALS,
  LESSGREATER,
  SHIFT,
  SUM,
  PRODUCT,
  PREFIX,
  CALL,
  INDEX,
};

static std::unordered_map<TokenType, Precedence> precedences = {
    {TokenType::Eq, EQUALS},      {TokenType::Neq, EQUALS},
    {TokenType::LT, LESSGREATER}, {TokenType::GT, LESSGREATER},
    {TokenType::Plus, SUM},       {TokenType::Minus, SUM},
    {TokenType::Slash, PRODUCT},  {TokenType::Asterisk, PRODUCT},
    {TokenType::LParen, CALL},    {TokenType::LBracket, INDEX},
    {TokenType::Xor, SINGLE},     {TokenType::Or, SINGLE},
    {TokenType::Amper, SINGLE},   {TokenType::LShift, SHIFT},
    {TokenType::RShift, SHIFT},
};

class Parser {
public:
  Parser(std::unique_ptr<LLexer> lx);
  std::unique_ptr<Program> parse_program();

private:
  std::unique_ptr<LLexer> lx_;
  LToken current_;
  LToken peek_;

  void next_token();

  StatementPtr parse_statement();
  StatementPtr parse_return_statement();
  StatementPtr parse_expression_statement();
  StatementPtr parse_var_decl();
  ExpressionPtr parse_for_expression();
  ExpressionPtr parse_while_expression();
  StatementPtr parse_global_decl();
  ExpressionPtr parse_primary();
  ExpressionPtr parse_prefix();
  ExpressionPtr parse_expression_rec(Precedence);
  ExpressionPtr parse_postfix();
  ExpressionPtr parse_array(ExpressionPtr);
  ExpressionPtr parse_call(ExpressionPtr);
  ExpressionPtr parse_identifier() const;
  ExpressionPtr parse_integer_literal();
  ExpressionPtr parse_if_expression();
  std::unique_ptr<BlockStatement> parse_block_statement();
  StatementPtr parse_function_literal();

  std::vector<ExpressionPtr> parse_expression_list();
  std::vector<std::unique_ptr<Identifier>> parse_function_params();
  ValueT parse_type();

  bool expect_peek(TokenType);
  bool peek_token_is(TokenType) const;
  bool current_token_is(TokenType) const;
};

#endif
