#include "lexer.h"
#include "token.h"
#include <cctype>
#include <stdlib.h>
#include <string.h>

Lexer::Lexer(const std::string &input) {
  input_ = input;
  line = 0;
  read_pos_ = 0;
  read_char();
}

static bool is_letter(char ch) {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }

void Lexer::read_char() {
  if (read_pos_ >= (int)input_.size())
    ch_ = 0;
  else
    ch_ = input_[read_pos_];

  pos_ = read_pos_;
  read_pos_++;
}

Token new_token(tokentypes type, char ch) {
  return Token{type, std::string(1, ch)};
}

tokentypes lookup_ident(const std::string &str) {
  auto keyword = TokenKeywords.find(str);
  if (keyword == TokenKeywords.end())
    return tokentypes::Ident;
  return keyword->second;
}

Token Lexer::next_token() {
  Token tok;

  skip_whitespace();

  switch (ch_) {
  case '=':
    if (peek_char() == '=') {
      tok = Token{.type = tokentypes::Eq, .literal = "=="};
      read_char();
    } else if (peek_char() == '>') {
      tok = Token{.type = tokentypes::Arrow, .literal = "=>"};
      read_char();
    } else {
      tok = new_token(tokentypes::Assign, ch_);
    }
    read_char();
    break;
  case ';':
    tok = new_token(tokentypes::Semicolon, ch_);
    read_char();
    break;
  case ':':
    tok = new_token(tokentypes::Colon, ch_);
    read_char();
    break;
  case '(':
    tok = new_token(tokentypes::LParen, ch_);
    read_char();
    break;
  case ')':
    tok = new_token(tokentypes::RParen, ch_);
    read_char();
    break;
  case '[':
    tok = new_token(tokentypes::LBracket, ch_);
    read_char();
    break;
  case ']':
    tok = new_token(tokentypes::RBracket, ch_);
    read_char();
    break;
  case ',':
    tok = new_token(tokentypes::Comma, ch_);
    read_char();
    break;
  case '+':
    if (peek_char() == '+') {
      tok = Token{.type = tokentypes::Inc, .literal = "++"};
      read_char();
    } else {
      tok = new_token(tokentypes::Plus, ch_);
    }
    read_char();
    break;
  case '-':
    if (peek_char() == '-') {
      tok = Token{.type = tokentypes::Minus, .literal = "--"};
      read_char();
    } else {
      tok = new_token(tokentypes::Minus, ch_);
    }
    read_char();
    break;
  case '!':
    if (peek_char() == '=') {
      tok = Token{.type = tokentypes::Neq, .literal = "!="};
      read_char();
    } else {
      tok = new_token(tokentypes::Bang, ch_);
    }
    read_char();
    break;
  case '/':
    tok = new_token(tokentypes::Slash, ch_);
    read_char();
    break;
  case '*':
    tok = new_token(tokentypes::Asterisk, ch_);
    read_char();
    break;
  case '<':
    if (peek_char() == '=') {
      tok = Token{.type = tokentypes::ELT, .literal = "<="};
      read_char();
    } else if (peek_char() == '<') {
      tok = Token{.type = tokentypes::LShift, .literal = "<<"};
      read_char();
    } else {
      tok = new_token(tokentypes::LT, ch_);
    }
    read_char();
    break;
  case '>':
    if (peek_char() == '=') {
      tok = Token{.type = tokentypes::EGT, .literal = ">="};
      read_char();
    } else if (peek_char() == '>') {
      tok = Token{.type = tokentypes::RShift, .literal = ">>"};
      read_char();
    } else {
      tok = new_token(tokentypes::GT, ch_);
    }
    read_char();
    break;
  case '{':
    tok = new_token(tokentypes::LBrace, ch_);
    read_char();
    break;
  case '}':
    tok = new_token(tokentypes::RBrace, ch_);
    read_char();
    break;
  case '^':
    tok = new_token(tokentypes::Xor, ch_);
    read_char();
    break;
  case '~':
    tok = new_token(tokentypes::Invert, ch_);
    read_char();
    break;
  case '&':
    if (peek_char() == '&') {
      tok = Token{.type = tokentypes::LogAnd, .literal = "&&"};
      read_char();
    } else {
      tok = new_token(tokentypes::Amper, ch_);
    }
    read_char();
    break;
  case '|':
    if (peek_char() == '|') {
      tok = Token{.type = tokentypes::LogOr, .literal = "||"};
      read_char();
    } else {
      tok = new_token(tokentypes::Or, ch_);
    }
    read_char();
    break;
  case '"':
    tok.type = tokentypes::String;
    tok.literal = read_string();
    read_char();
    break;
  case 0:
    tok.literal = "";
    tok.type = tokentypes::Eof;
    read_char();
    break;
  default:
    if (is_letter(ch_)) {
      tok.literal = read_ident();
      tok.type = lookup_ident(tok.literal);
    } else if (is_digit(ch_)) {
      tok.type = tokentypes::Int;
      tok.literal = read_number();
    } else {
      tok = new_token(tokentypes::Illegal, ch_);
      read_char();
    }
    break;
  }

  return tok;
}

std::string Lexer::read_string() {
  int start_pos = pos_ + 1;
  for (;;) {
    read_char();
    if (ch_ == '"' || ch_ == 0) {
      break;
    }
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

void Lexer::skip_whitespace() {
  while (ch_ == ' ' || ch_ == '\t' || ch_ == '\n' || ch_ == '\r') {
    if (ch_ == '\n')
      ++line;

    read_char();
  }
}

std::string Lexer::read_ident() {
  int start_pos = pos_;
  while (is_letter(ch_)) {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

std::string Lexer::read_number() {
  int start_pos = pos_;
  while (is_digit(ch_)) {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

char Lexer::peek_char() {
  if (read_pos_ >= (int)input_.size())
    return 0;
  else
    return input_[read_pos_];
}

LToken LLexer::next_token() {
  skip();

  start_ = curr_;
  if (is_at_end())
    return make_token(tokentypes::Eof);

  auto c = advance();
  if (std::isalpha(c))
    return ident();
  if (std::isdigit(c))
    return num();

  switch (c) {
  case '(':
    return make_token(tokentypes::LParen);
  case ')':
    return make_token(tokentypes::RParen);
  case '{':
    return make_token(tokentypes::LBrace);
  case '}':
    return make_token(tokentypes::RBrace);
  case ';':
    return make_token(tokentypes::Semicolon);
  case ',':
    return make_token(tokentypes::Comma);
  case '-':
    return make_token(tokentypes::Minus);
  case '+':
    return make_token(tokentypes::Plus);
  case '/':
    return make_token(tokentypes::Slash);
  case '*':
    return make_token(tokentypes::Asterisk);
  case '&':
    return make_token(tokentypes::Amper);
  case '!':
    return make_token(match('=') ? tokentypes::Neq : tokentypes::Bang);
  case '=':
    return make_token(match('=') ? tokentypes::Eq : tokentypes::Assign);
  case '<':
    return make_token(match('=') ? tokentypes::ELT : tokentypes::LT);
  case '>':
    return make_token(match('=') ? tokentypes::EGT : tokentypes::GT);
  case '"':
    return str();
  }
  return error_token("Unexpected character.");
}

LToken LLexer::ident() {
  while (std::isalnum(peek()))
    advance();
  return make_token(ident_type());
}

LToken LLexer::num() {
  while (std::isdigit(peek()))
    advance();

  if (peek() == '.' && std::isdigit(peek_next())) {
    advance();
    while (std::isdigit(peek()))
      advance();
  }

  return make_token(tokentypes::Int);
}

LToken LLexer::str() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n')
      ++line_;
    advance();
  }

  if (is_at_end())
    return error_token("unterminated string");

  advance();
  return make_token(tokentypes::String);
}

void LLexer::skip() {
  while (true) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      ++line_;
      advance();
      break;
    case '/':
      if (peek_next() == '/') {
        while (peek() != '\n' && !is_at_end())
          advance();
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

tokentypes LLexer::ident_type() const {
  switch (src.at(start_)) {
  case 'e':
    return check_keyword(1, 3, "lse", tokentypes::Else);
  case 'f':
    if (curr_ - start_ > 1) {
      switch (src.at(start_ + 1)) {
      case 'a':
        return check_keyword(2, 3, "lse", tokentypes::False);
      case 'o':
        return check_keyword(2, 1, "r", tokentypes::For);
      case 'n':
        return tokentypes::Function;
      }
    }
    break;
  case 'i':
    return check_keyword(1, 1, "f", tokentypes::If);
  case 'r':
    return check_keyword(1, 5, "eturn", tokentypes::Return);
  case 'v':
    return check_keyword(1, 2, "ar", tokentypes::Var);
  case 'w':
    return check_keyword(1, 4, "hile", tokentypes::While);
  default:
    break;
  }

  return tokentypes::Ident;
}

tokentypes LLexer::check_keyword(size_t begin, size_t length,
                                 std::string_view rest, tokentypes type) const {
  if (curr_ - start_ == begin + length &&
      src.substr(start_ + begin, length) == rest)
    return type;

  return tokentypes::Ident;
}

LToken LLexer::error_token(std::string_view message) const noexcept {
  return LToken(tokentypes::Eof, message, line_);
}

LToken LLexer::make_token(tokentypes type) const noexcept {
  size_t length = curr_ - start_;
  return LToken(type, src.substr(start_, length), line_);
}

char LLexer::advance() {
  ++curr_;
  return src.at(curr_ - 1);
}

bool LLexer::is_at_end() const noexcept { return curr_ == src.length(); }

bool LLexer::match(char expected) {
  if (is_at_end())
    return false;
  if (src.at(curr_) != expected)
    return false;
  ++curr_;

  return true;
}

char LLexer::peek() const {
  if (is_at_end())
    return '\0';
  return src.at(curr_);
}

char LLexer::peek_next() const {
  if (is_at_end() || curr_ + 1 == src.length())
    return '\0';

  return src.at(curr_ + 1);
}
