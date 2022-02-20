#include "token.h"

#include <cctype>
#include <cstdlib>
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

static bool starts_with(char *p, char *start) {
  return strncmp(p, start, strlen(start)) == 0;
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
      continue;
    }
  }

  return res;
}
} // namespace token
