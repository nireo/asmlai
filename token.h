#ifndef _ASMLAI_TOKEN_H
#define _ASMLAI_TOKEN_H

#include "types.h"
#include <memory.h>
#include <string>
#include <variant>
#include <vector>

namespace token {

enum class TokenType {
  Num,
  Eof,
  Common,
  Identifier,
  Keyword,
};

struct Token {
  bool operator==(const char *val) const {
    return memcmp(this->loc_, val, this->len_) == 0 && val[this->len_] == '\0';
  }

  TokenType type_;
  // variat because in the future there will be more stuff here.
  std::variant<std::monostate, i64> data_;
  i64 len_{0};
  char *loc_ = nullptr;
};

template <typename... Args> void error(const char *format_string, Args... args);
template <typename... Args>
void error_at(char *location, const char *format_string, Args... args);

std::vector<Token> tokenize_input(char *p);

} // namespace token

#endif
