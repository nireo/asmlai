#ifndef LUPS_LEXER_H
#define LUPS_LEXER_H

#include "token.h"
#include <string>

class Lexer
{
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
  int line;
  int length;
  const char *start;
  tokentypes type;
};

struct CLexer {
  int line;
  const char *start;
  const char *current;

  char advance();
  bool is_at_end();
  bool match(char expected);
  LToken make_token(tokentypes);
  LToken error_token(const char *);
  char peek();
  char peek_next();
  void skip_whitespace();
  LToken check_keyword(int, int, const char *, tokentypes);
  LToken identifier_type();
  LToken number();
  LToken identifier();
  LToken string();
  LToken scan_token();
};

#endif
