#include "token.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <variant>
#include <vector>

namespace token {
static char *curr_input;
static char *curr_filename;

template <typename... Args>
void error(const char *format_string, Args... args) {
  std::fprintf(stderr, format_string, args...);
  std::fprintf(stderr, "\n");
  return;
}

template <typename... Args>
void error_at_(int line_number, char *location, const char *format_string,
               Args... args) {
  char *line = location;
  while (curr_input < line && line[-1] != '\n')
    line--;

  char *end = location;
  while (*end != '\n')
    end++;

  int indent = fprintf(stderr, "%s:%d: ", curr_filename, line_number);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  int pos = location - line + indent;

  std::fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  std::fprintf(stderr, "^ ");
  std::fprintf(stderr, format_string, args...);
  std::fprintf(stderr, "\n");

  std::exit(1);
}

template <typename... Args>
void error_at(char *loc, const char *fmt, Args... args) {
  int line_number = 1;
  for (char *p = curr_input; p < loc; p++) {
    if (*p == '\n')
      ++line_number;
  }
  error_at_(line_number, loc, fmt, args...);
}

template <typename... Args>
void error_token(const Token &tok, const char *format_string, Args... args) {
  error_at_(tok.line_number_, tok.loc_, format_string, args...);
}

static Token new_token(char *start, char *end, TokenType type_) {
  return Token{
      .type_ = type_,
      .data_ = std::monostate{},
      .len_ = end - start,
      .loc_ = start,
  };
}

static void add_line_numbers(std::vector<Token> &tokens) {
  char *p = curr_input;
  i32 num = 1, pos = 0;

  do {
    if (p == tokens[pos].loc_) {
      tokens[pos].line_number_ = num;
      ++pos;
    }

    if (*p == '\n')
      ++num;
  } while (*p++);
}

static bool is_ident_char(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static int from_hex(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  return c - 'A' + 10;
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

  constexpr const char *kw[] = {"==", "!=", "<=", ">=", "->"};
  for (int i = 0; i < 5; ++i) {
    if (starts_with(p, kw[i])) {
      return strlen(kw[i]);
    }
  }

  return std::ispunct(*p) ? 1 : 0;
}

static int read_escaped_char(char **new_pos, char *p) {
  if ('0' <= *p && *p <= '7') {
    // octal
    int c = *p++ - '0';
    if ('0' <= *p && *p <= '7') {
      c = (c << 3) + (*p++ - '0');
      if ('0' <= *p && *p <= '7') {
        c = (c << 3) + (*p++ - '0');
      }
    }
    *new_pos = p;
    return c;
  }

  if (*p == 'x') {
    ++p;
    if (!std::isxdigit(*p))
      error_at(p, "invalid hex escape.");

    int c = 0;
    for (; std::isxdigit(*p); ++p)
      c = (c << 4) + from_hex(*p);
    *new_pos = p;
    return c;
  }

  *new_pos = p + 1;

  switch (*p) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
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
  case 'e':
    return 27;
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
  char *end = string_literal_end(start + 1);
  char *buffer = (char *)malloc((end - start) * sizeof(char));
  i64 len = 0;

  for (char *p = start + 1; p < end;) {
    if (*p == '\\') {
      buffer[len++] = read_escaped_char(&p, p + 1);
    } else {
      buffer[len++] = *p++;
    }
  }

  auto tok = new_token(start, end + 1, TokenType::String);
  StringLiteral lit{};
  lit.length = len + 1;
  lit.data = buffer;
  tok.data_ = std::move(lit);

  return tok;
}

std::vector<Token> tokenize_input(char *filename, char *p) {
  curr_filename = filename;
  curr_input = p;

  std::vector<Token> res;

  while (*p) {
    if (starts_with(p, "//")) {
      p += 2;
      while (*p != '\n')
        ++p;
      continue;
    }

    if (starts_with(p, "/*")) {
      char *q = strstr(p + 2, "*/"); // kinda scuffed hack
      if (!q) {
        error_at(p, "unclosed block comment");
      }
      p = q + 2;
      continue;
    }

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

  add_line_numbers(res);
  return res;
}

static char *file_to_string(char *fpath) {
  FILE *fp;
  if (strcmp(fpath, "-") == 0) {
    fp = stdin;
  } else {
    fp = std::fopen(fpath, "r");
    if (!fp)
      error("cannot open %s: %s", fpath, strerror(errno));
  }

  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  for (;;) {
    char buf2[4096];
    int n = std::fread(buf2, 1, sizeof(buf2), fp);
    if (n == 0)
      break;
    std::fwrite(buf2, 1, n, out);
  }

  if (fp != stdin)
    std::fclose(fp);

  std::fflush(out);
  if (buflen == 0 || buf[buflen - 1] != '\n')
    std::fputc('\n', out);
  std::fputc('\0', out);
  std::fclose(out);
  return buf;
}

std::vector<Token> tokenize_path(char *path) {
  return tokenize_input(path, file_to_string(path));
}
} // namespace token
