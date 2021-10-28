#include "lexer.h"
#include "token.h"
#include <cctype>
#include <iostream>
#include <stdlib.h>
#include <string.h>

LToken LLexer::next_token() {
  skip();

  start_ = curr_;
  if (is_at_end())
    return make_token(TokenType::Eof);

  auto c = advance();
  if (std::isalpha(c))
    return ident();
  if (std::isdigit(c))
    return num();

  std::cout << c << '\n';

  switch (c) {
  case '(':
    return make_token(TokenType::LParen);
  case ')':
    return make_token(TokenType::RParen);
  case '{':
    return make_token(TokenType::LBrace);
  case '}':
    return make_token(TokenType::RBrace);
  case ';':
    return make_token(TokenType::Semicolon);
  case '|':
    return make_token(match('|') ? TokenType::LogOr : TokenType::Or);
  case ',':
    return make_token(TokenType::Comma);
  case '-':
    return make_token(match('-') ? TokenType::Dec : TokenType::Minus);
  case '+':
    return make_token(match('+') ? TokenType::Inc : TokenType::Plus);
  case '/':
    return make_token(TokenType::Slash);
  case '*':
    return make_token(TokenType::Asterisk);
  case '&':
    return make_token(TokenType::Amper);
  case '~':
    return make_token(TokenType::Invert);
  case '!':
    return make_token(match('=') ? TokenType::Neq : TokenType::Bang);
  case '=':
    if (match('>')) {
      return make_token(TokenType::Arrow);
    } else if (match('=')) {
      return make_token(TokenType::Eq);
    }

    return make_token(TokenType::Assign);
  case '^':
    return make_token(TokenType::Xor);
  case '<':
    if (match('=')) {
      return make_token(TokenType::ELT);
    } else if ('<') {
      return make_token(TokenType::LShift);
    }

    return make_token(TokenType::LT);
  case '>':
    if (match('=')) {
      return make_token(TokenType::ELT);
    } else if ('>') {
      return make_token(TokenType::RShift);
    }

    return make_token(TokenType::GT);
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

  return make_token(TokenType::Int);
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
  return make_token(TokenType::String);
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

TokenType LLexer::ident_type() const {
  switch (src.at(start_)) {
  case 'e':
    return check_keyword(1, 3, "lse", TokenType::Else);
  case 'f':
    if (curr_ - start_ > 1) {
      switch (src.at(start_ + 1)) {
      case 'a':
        return check_keyword(2, 3, "lse", TokenType::False);
      case 'o':
        return check_keyword(2, 1, "r", TokenType::For);
      case 'u':
        return check_keyword(2, 2, "nc", TokenType::Function);
      }
    }
    break;
  case 'i':
    if (curr_ - start_ > 1) {
      switch (src.at(start_ + 1)) {
      case 'n':
        return check_keyword(2, 1, "t", TokenType::IntType);
      case 'f':
        return TokenType::If;
      }
    }
    break;
  case 'c':
    return check_keyword(1, 3, "har", TokenType::CharType);
  case 'r':
    return check_keyword(1, 5, "eturn", TokenType::Return);
  case 'l':
    return check_keyword(1, 2, "et", TokenType::Let);
  case 'v':
    if (curr_ - start_ > 1) {
      switch (src.at(start_ + 1)) {
      case 'a':
        return check_keyword(2, 1, "r", TokenType::Var);
      case 'o':
        return check_keyword(2, 2, "id", TokenType::Void);
      }
    }
    break;
  case 'g':
    return check_keyword(1, 5, "lobal", TokenType::Global);
  case 's':
    return check_keyword(1, 5, "tring", TokenType::StringType);
  case 'w':
    return check_keyword(1, 4, "hile", TokenType::While);
  default:
    break;
  }

  return TokenType::Ident;
}

TokenType LLexer::check_keyword(size_t begin, size_t length, std::string rest,
                                TokenType type) const {
  if (curr_ - start_ == begin + length &&
      src.substr(start_ + begin, length) == rest)
    return type;

  return TokenType::Ident;
}

LToken LLexer::error_token(std::string message) const noexcept {
  return LToken(TokenType::Eof, message, line_);
}

LToken LLexer::make_token(TokenType type) const noexcept {
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
