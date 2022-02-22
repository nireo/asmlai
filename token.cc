#include "token.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <variant>
#include <vector>

namespace token {
static char *curr_input;

template <typename... Args>
void error(const char *format_string, Args... args) {
  std::fprintf(stderr, format_string, args...);
  std::fprintf(stderr, "\n");
  return;
}

template <typename... Args>
void error_at(char *location, const char *format_string, Args... args) {
  i64 pos = location - curr_input;

  std::fprintf(stderr, "%s\n", curr_input);
  std::fprintf(stderr, "%*s", pos, "");
  std::fprintf(stderr, "^ ");
  std::fprintf(stderr, format_string, args...);
  std::fprintf(stderr, "\n");

  std::exit(1);
}

template <typename... Args>
void error_token(const Token &tok, const char *format_string, Args... args) {
  error_at(tok.loc, format_string, args...);
}

static Token new_token(char *start, char *end, TokenType type_) {
  return Token{
      .type_ = type_,
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

    if ('a' <= *p && *p <= 'z') {
      auto tok = new_token(p, p + 1, TokenType::Identifier);
      res.push_back(tok);
      p++;
      continue;
    }

    int p_len = read_punctuator(p);
    if (p_len) {
      auto tok = new_token(p, p + p_len, TokenType::Common);
      p += tok.len_;
      res.push_back(tok);
      continue;
    }

    std::cerr << "invalid token\n";
  }

  res.push_back(new_token(p, p, TokenType::Eof));
  return res;
}
} // namespace token
