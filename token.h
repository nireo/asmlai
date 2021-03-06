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
  String,
};

struct StringLiteral {
  u64 length;
  char *data;
};

struct Token {
  bool operator==(const char *val) const {
    return memcmp(this->loc_, val, this->len_) == 0 && val[this->len_] == '\0';
  }
  bool operator!=(const char *val) const { return !(*this == val); }

  TokenType type_;
  // variat because in the future there will be more stuff here.
  std::variant<std::monostate, i64, StringLiteral> data_;
  i64 len_{0};
  char *loc_ = nullptr;
  i32 line_number_{0};
};

template <typename... Args> void error(const char *format_string, Args... args);
template <typename... Args>
void error_at(char *location, const char *format_string, Args... args);

std::vector<Token> tokenize_input(char *filename, char *p);
std::vector<Token> tokenize_path(char *path);

} // namespace token

#endif
