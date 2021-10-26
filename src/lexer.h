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
  tokentypes type;
  size_t line;
  std::string_view literal;
};

#endif
