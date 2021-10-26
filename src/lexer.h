#ifndef LUPS_LEXER_H
#define LUPS_LEXER_H

#include "token.h"
#include <string>
#include <string_view>

class Lexer {
public:
  Lexer(const std::string &input);
  Token next_token();
  int line;

private:
  std::string input_;
  int pos_;
  char ch_;
  int read_pos_;

  std::string read_ident();
  std::string read_number();
  std::string read_string();

  void read_char();
  void skip_whitespace();

  char peek_char();
};

struct LToken {
  tokentypes type_;
  size_t line_ = 1;
  std::string_view literal_;

  constexpr LToken() noexcept : type_(tokentypes::Eof){};
  constexpr LToken(tokentypes t, std::string_view s, size_t ln) noexcept
      : type_(t), line_(ln), literal_(s){};
};

struct LLexer {
  size_t line_ = 1;
  size_t start_ = 0;
  size_t curr_ = 0;
  std::string_view src;

  constexpr explicit LLexer(std::string_view src) noexcept : src(src) {}
  LToken next_token();

private:
  LToken ident();
  LToken num();
  LToken str();
  void skip();

  tokentypes ident_type() const;
  tokentypes check_keyword(size_t begin, size_t length, std::string_view rest,
                           tokentypes type) const;
  LToken error_token(std::string_view message) const noexcept;
  LToken make_token(tokentypes type) const noexcept;

  char advance();
  bool is_at_end() const noexcept;
  bool match(char expected);
  char peek() const;
  char peek_next() const;
};

#endif
