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
  error_at(tok.loc_, format_string, args...);
}

static Token new_token(char *start, char *end, TokenType type_) {
  return Token{
      .type_ = type_,
      .data_ = std::monostate{},
      .len_ = end - start,
      .loc_ = start,
  };
}

static bool is_ident_char(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_ident_any(char c) {
  return is_ident_char(c) || ('0' <= c && c <= '9');
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

static int read_escaped_char(char *p) {
  switch (*p) {
  case 'a':
    return '\a';
  case 'b':
    return 'b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  default:
    return *p;
  }
}

static char *string_literal_end(char *p) {
  char *start = p;
  for (; *p != '"'; p++) {
    if (*p == '\n' || *p == '\0') {
      error_at(start, "unclosed string litreal");
    }

    if (*p == '\\') {
      p++;
    }
  }
  return p;
}

static Token read_string(char *start) {
  char *ptr = start + 1;
  for (; *ptr != '"'; ptr++) {
    if (*ptr == '\n' || *ptr == '\0') {
      error_at(start, "unterminated string");
    }
  }

  auto tok = new_token(start, ptr + 1, TokenType::String);
  StringLiteral lit{};
  lit.length = ptr - start;
  lit.data = strndup(start + 1, ptr - start - 1);
  tok.data_ = std::move(lit);

  return tok;
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

    if (*p == '"') {
      auto tok = read_string(p);
      p += tok.len_;
      res.push_back(std::move(tok));
      continue;
    }

    if (is_ident_char(*p)) {
      char *start = p;
      do {
        p++;
      } while (is_ident_any(*p));
      auto tok = new_token(start, p, TokenType::Identifier);
      res.push_back(std::move(tok));
      continue;
    }

    int p_len = read_punctuator(p);
    if (p_len) {
      auto tok = new_token(p, p + p_len, TokenType::Common);
      p += tok.len_;
      res.push_back(std::move(tok));
      continue;
    }

    std::cerr << "invalid token\n";
  }

  for (auto &tok : res) {
    if (tok == "return" || tok == "else" || tok == "if" || tok == "for") {
      tok.type_ = TokenType::Keyword;
    }
  }

  res.push_back(new_token(p, p, TokenType::Eof));
  return res;
}
} // namespace token
