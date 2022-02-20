#ifndef _ASMLAI_TOKEN_H
#define _ASMLAI_TOKEN_H

#include "types.h"
#include <memory.h>
#include <string>
#include <variant>

namespace token {

enum class TokenType {
  Num,
  Eof,
  Common,
};

struct Token {
  bool operator==(const char *val) const {
    return memcmp(this->loc, val, this->len_) == 0 && val[this->len_] == '\0';
  }

  TokenType type_;
  // variat because in the future there will be more stuff here.
  std::variant<std::monostate, i64> data_;
  i64 len_{0};
  char *loc = nullptr;
};

} // namespace token

#endif
