#ifndef LUPS_PARSER_H
#define LUPS_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include <memory>
#include <unordered_map>

class Parser;
typedef std::unique_ptr<Expression> (*PrefixParseFn)();
typedef std::unique_ptr<Expression> (*InfixParseFn)(
    std::unique_ptr<Expression>);

enum Precedence {
  LOWEST,
  EQUALS,
  LESSGREATER,
  SUM,
  PRODUCT,
  PREFIX,
  CALL,
  INDEX,
};

class Parser
{
public:
  Parser(std::unique_ptr<Lexer> lx);
  Parser() : lx_(nullptr) {}
  std::unique_ptr<Program> parse_program();
  std::vector<std::string> errors() const;

  std::unique_ptr<Lexer> lx_;
  Token current_;
  Token peek_;

  std::unordered_map<tokentypes, PrefixParseFn> m_prefix_parse_fns;
  std::unordered_map<tokentypes, InfixParseFn> m_infix_parse_fns;
  void add_prefix_parse(tokentypes tt, PrefixParseFn fn);
  void add_infix_parse(tokentypes tt, InfixParseFn fn);

  void next_token();

  Precedence peek_precedence();
  Precedence current_precedence();

  bool expect_peek(tokentypes tt);
  bool peek_token_is(tokentypes tt);
  bool current_token_is(tokentypes tt);
  void peek_error(tokentypes tt);

  std::vector<std::string> errors_;
};

std::unique_ptr<Program> parse_source(std::string source);

#endif
