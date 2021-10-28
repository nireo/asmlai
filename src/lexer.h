#ifndef LUPS_LEXER_H
#define LUPS_LEXER_H

#include "token.h"
#include <string>
#include <string_view>

struct LToken {
  TokenType type;
  size_t line_ = 1;
  std::string literal_;

  LToken() noexcept : type(TokenType::Eof){};
  LToken(TokenType t, std::string s, size_t ln) noexcept
      : type(t), line_(ln), literal_(s){};
};

struct LLexer {
  size_t line_ = 1;
  size_t start_ = 0;
  size_t curr_ = 0;
  std::string src;

  LLexer(std::string src) noexcept : src(src) {}
  LToken next_token();

private:
  LToken ident();
  LToken num();
  LToken str();
  void skip();

  TokenType ident_type() const;
  TokenType check_keyword(size_t begin, size_t length, std::string rest,
                          TokenType type) const;
  LToken error_token(std::string message) const noexcept;
  LToken make_token(TokenType type) const noexcept;

  char advance();
  bool is_at_end() const noexcept;
  bool match(char expected);
  char peek() const;
  char peek_next() const;
};

#endif
