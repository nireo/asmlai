#ifndef _ASMLAI_TOKEN_H
#define _ASMLAI_TOKEN_H

#include "types.h"
#include <string>
#include <variant>

namespace token {

enum class TokenType {
  Num,
  Eof,
  Common,
};

struct Token {
  TokenType type_;

  // variat because in the future there will be more stuff here.
  std::variant<std::monostate, i64> data_;
  i64 len_{0};
  char *loc = nullptr;
};

} // namespace token

#endif
