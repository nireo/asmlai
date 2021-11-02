#ifndef LAI_PARSER_H
#define LAI_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include <memory>
#include <unordered_map>

class Parser;

enum Precedence
{
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
    {TokenType::Eq, EQUALS},
    {TokenType::Neq, EQUALS},
    {TokenType::LT, LESSGREATER},
    {TokenType::GT, LESSGREATER},
    {TokenType::Plus, SUM},
    {TokenType::Minus, SUM},
    {TokenType::Slash, PRODUCT},
    {TokenType::Asterisk, PRODUCT},
    {TokenType::LParen, CALL},
    {TokenType::LBracket, INDEX},
    {TokenType::Xor, SINGLE},
    {TokenType::Or, SINGLE},
    {TokenType::Amper, SINGLE},
    {TokenType::LShift, SHIFT},
    {TokenType::RShift, SHIFT},
};

class Parser
{
public:
  Parser(std::unique_ptr<LLexer> lx);
  std::unique_ptr<Program> parse_program();

private:
  std::unique_ptr<LLexer> lx_;
  LToken current_;
  LToken peek_;

  void next_token();

  std::unique_ptr<Statement> parse_statement();
  std::unique_ptr<Statement> parse_return_statement();
  std::unique_ptr<Statement> parse_expression_statement();
  std::unique_ptr<Statement> parse_var_decl();
  std::unique_ptr<Expression> parse_for_expression();
  std::unique_ptr<Expression> parse_while_expression();
  std::unique_ptr<Statement> parse_global_decl();
  std::unique_ptr<Expression> parse_primary();
  std::unique_ptr<Expression> parse_prefix();
  std::unique_ptr<Expression> parse_expression_rec(Precedence prec);
  std::unique_ptr<Expression> parse_postfix();
  std::unique_ptr<Expression> parse_array(std::unique_ptr<Expression> ident);
  std::unique_ptr<Expression> parse_call(std::unique_ptr<Expression> ident);
  std::unique_ptr<Expression> parse_identifier();
  std::unique_ptr<Expression> parse_integer_literal();
  std::unique_ptr<Expression> parse_if_expression();
  std::unique_ptr<BlockStatement> parse_block_statement();
  std::unique_ptr<Statement> parse_function_literal();

  std::vector<std::unique_ptr<Expression>> parse_expression_list();
  std::vector<std::unique_ptr<Identifier>> parse_function_params();
  ValueT parse_type();

  Precedence peek_precedence();
  Precedence current_precedence();

  bool expect_peek(TokenType tt);
  bool peek_token_is(TokenType tt);
  bool current_token_is(TokenType tt);
};

#endif
