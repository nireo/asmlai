#include "token.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <variant>
#include <vector>

namespace token {
static char *curr_input;

static Token new_token(char *start, char *end, TokenType type) {
  return Token{
      .type_ = type,
      .data_ = std::monostate{},
      .len_ = end - start,
      .loc = start,
  };
}

static bool starts_with(char *p, const char *start) {
  return strncmp(p, start, strlen(start)) == 0;
}

static int read_punctuator(char *p) {
  if (starts_with(p, "==") || starts_with(p, "!=") || starts_with(p, "<=") ||
      starts_with(p, ">="))
    return 2;

  return std::ispunct(*p) ? 1 : 0;
}

std::vector<Token> tokenize_input(char *p) {
  std::vector<Token> res;

  while (*p) {
    if (std::isspace(*p)) {
      p++;
      continue;
    }

    if (std::isdigit(*p)) {
      char *ss = p;
      Token tok = new_token(p, p, TokenType::Num);
      tok.data_ = static_cast<i64>(strtoul(p, &p, 10));
      tok.len_ = p - ss;
      res.push_back(std::move(tok));

      continue;
    }

    int p_len = read_punctuator(p);
    if (p_len) {
      auto tok = new_token(p, p + p_len, TokenType::Common);
      p += tok.len_;
      continue;
    }
    std::cerr << "invalid token\n";
  }

  return res;
}
} // namespace token
