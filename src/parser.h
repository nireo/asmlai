#ifndef LAI_PARSER_H
#define LAI_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include <memory>
#include <unordered_map>

class Parser;
typedef std::unique_ptr<Expression> (Parser::*PrefixParseFn)();
typedef std::unique_ptr<Expression> (Parser::*InfixParseFn)(
    std::unique_ptr<Expression>);

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

static std::unordered_map<tokentypes, Precedence> precedences = {
  { tokentypes::Eq, EQUALS },      { tokentypes::Neq, EQUALS },
  { tokentypes::LT, LESSGREATER }, { tokentypes::GT, LESSGREATER },
  { tokentypes::Plus, SUM },       { tokentypes::Minus, SUM },
  { tokentypes::Slash, PRODUCT },  { tokentypes::Asterisk, PRODUCT },
  { tokentypes::LParen, CALL },    { tokentypes::LBracket, INDEX },
  { tokentypes::Xor, SINGLE },     { tokentypes::Or, SINGLE },
  { tokentypes::Amper, SINGLE },   { tokentypes::LShift, SHIFT },
  { tokentypes::RShift, SHIFT },
};

class Parser
{
public:
  Parser(std::unique_ptr<Lexer> lx);
  std::unique_ptr<Program> parse_program();
  std::vector<std::string> errors() const;

private:
  std::unique_ptr<Lexer> lx_;
  Token current_;
  Token peek_;

  std::unordered_map<tokentypes, PrefixParseFn> m_prefix_parse_fns;
  std::unordered_map<tokentypes, InfixParseFn> m_infix_parse_fns;
  void add_prefix_parse(tokentypes tt, PrefixParseFn fn);
  void add_infix_parse(tokentypes tt, InfixParseFn fn);

  void next_token();

  std::unique_ptr<Statement> parse_statement();
  std::unique_ptr<Statement> parse_let_statement();
  std::unique_ptr<Statement> parse_return_statement();
  std::unique_ptr<Statement> parse_expression_statement();
  std::unique_ptr<Expression> parse_for_expression();
  std::unique_ptr<Expression> parse_while_expression();
  std::unique_ptr<Statement> parse_assingment();
  std::unique_ptr<Statement> parse_global_decl();
  std::unique_ptr<Expression> parse_primary();
  std::unique_ptr<Expression> parse_prefix();
  std::unique_ptr<Expression> parse_expression_rec(Precedence prec);
  std::unique_ptr<Expression> parse_postfix();
  std::unique_ptr<Expression> parse_array();
  std::unique_ptr<Expression> parse_call();
  std::pair<std::unique_ptr<Expression>, valuetype>
  parse_expression(Precedence prec);
  std::unique_ptr<Expression> parse_identifier();
  std::unique_ptr<Expression> parse_integer_literal();
  std::unique_ptr<Expression> parse_prefix_expression();
  std::unique_ptr<Expression>
  parse_infix_expression(std::unique_ptr<Expression> left);
  std::unique_ptr<Expression> parse_boolean();
  std::unique_ptr<Expression> parse_grouped_expression();
  std::unique_ptr<Expression> parse_if_expression();
  std::unique_ptr<BlockStatement> parse_block_statement();
  std::unique_ptr<Statement> parse_function_literal();
  std::unique_ptr<Expression> parse_string_literal();

  std::vector<std::unique_ptr<Identifier> > parse_function_params();
  valuetype parse_type();

  Precedence peek_precedence();
  Precedence current_precedence();

  bool expect_peek(tokentypes tt);
  bool peek_token_is(tokentypes tt);
  bool current_token_is(tokentypes tt);
};

#endif
