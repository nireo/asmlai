#include "lexer.h"
#include "token.h"
#include <cctype>
#include <iostream>
#include <stdlib.h>
#include <string.h>

Lexer::Lexer(const std::string &input)
{
  input_ = input;
  line = 0;
  read_pos_ = 0;
  read_char();
}

static bool is_letter(char ch)
{
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }

void Lexer::read_char()
{
  if (read_pos_ >= (int)input_.size())
    ch_ = 0;
  else
    ch_ = input_[read_pos_];

  pos_ = read_pos_;
  read_pos_++;
}

Token new_token(TokenType type, char ch)
{
  return Token{type, std::string(1, ch)};
}

TokenType lookup_ident(const std::string &str)
{
  auto keyword = TokenKeywords.find(str);
  if (keyword == TokenKeywords.end())
    return TokenType::Ident;
  return keyword->second;
}

Token Lexer::next_token()
{
  Token tok;

  skip_whitespace();

  switch (ch_)
  {
  case '=':
    if (peek_char() == '=')
    {
      tok = Token{.type = TokenType::Eq, .literal = "=="};
      read_char();
    }
    else if (peek_char() == '>')
    {
      tok = Token{.type = TokenType::Arrow, .literal = "=>"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Assign, ch_);
    }
    read_char();
    break;
  case ';':
    tok = new_token(TokenType::Semicolon, ch_);
    read_char();
    break;
  case ':':
    tok = new_token(TokenType::Colon, ch_);
    read_char();
    break;
  case '(':
    tok = new_token(TokenType::LParen, ch_);
    read_char();
    break;
  case ')':
    tok = new_token(TokenType::RParen, ch_);
    read_char();
    break;
  case '[':
    tok = new_token(TokenType::LBracket, ch_);
    read_char();
    break;
  case ']':
    tok = new_token(TokenType::RBracket, ch_);
    read_char();
    break;
  case ',':
    tok = new_token(TokenType::Comma, ch_);
    read_char();
    break;
  case '+':
    if (peek_char() == '+')
    {
      tok = Token{.type = TokenType::Inc, .literal = "++"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Plus, ch_);
    }
    read_char();
    break;
  case '-':
    if (peek_char() == '-')
    {
      tok = Token{.type = TokenType::Minus, .literal = "--"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Minus, ch_);
    }
    read_char();
    break;
  case '!':
    if (peek_char() == '=')
    {
      tok = Token{.type = TokenType::Neq, .literal = "!="};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Bang, ch_);
    }
    read_char();
    break;
  case '/':
    tok = new_token(TokenType::Slash, ch_);
    read_char();
    break;
  case '*':
    tok = new_token(TokenType::Asterisk, ch_);
    read_char();
    break;
  case '<':
    if (peek_char() == '=')
    {
      tok = Token{.type = TokenType::ELT, .literal = "<="};
      read_char();
    }
    else if (peek_char() == '<')
    {
      tok = Token{.type = TokenType::LShift, .literal = "<<"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::LT, ch_);
    }
    read_char();
    break;
  case '>':
    if (peek_char() == '=')
    {
      tok = Token{.type = TokenType::EGT, .literal = ">="};
      read_char();
    }
    else if (peek_char() == '>')
    {
      tok = Token{.type = TokenType::RShift, .literal = ">>"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::GT, ch_);
    }
    read_char();
    break;
  case '{':
    tok = new_token(TokenType::LBrace, ch_);
    read_char();
    break;
  case '}':
    tok = new_token(TokenType::RBrace, ch_);
    read_char();
    break;
  case '^':
    tok = new_token(TokenType::Xor, ch_);
    read_char();
    break;
  case '~':
    tok = new_token(TokenType::Invert, ch_);
    read_char();
    break;
  case '&':
    if (peek_char() == '&')
    {
      tok = Token{.type = TokenType::LogAnd, .literal = "&&"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Amper, ch_);
    }
    read_char();
    break;
  case '|':
    if (peek_char() == '|')
    {
      tok = Token{.type = TokenType::LogOr, .literal = "||"};
      read_char();
    }
    else
    {
      tok = new_token(TokenType::Or, ch_);
    }
    read_char();
    break;
  case '"':
    tok.type = TokenType::String;
    tok.literal = read_string();
    read_char();
    break;
  case 0:
    tok.literal = "";
    tok.type = TokenType::Eof;
    read_char();
    break;
  default:
    if (is_letter(ch_))
    {
      tok.literal = read_ident();
      tok.type = lookup_ident(tok.literal);
    }
    else if (is_digit(ch_))
    {
      tok.type = TokenType::Int;
      tok.literal = read_number();
    }
    else
    {
      tok = new_token(TokenType::Illegal, ch_);
      read_char();
    }
    break;
  }

  return tok;
}

std::string Lexer::read_string()
{
  int start_pos = pos_ + 1;
  for (;;)
  {
    read_char();
    if (ch_ == '"' || ch_ == 0)
    {
      break;
    }
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

void Lexer::skip_whitespace()
{
  while (ch_ == ' ' || ch_ == '\t' || ch_ == '\n' || ch_ == '\r')
  {
    if (ch_ == '\n')
      ++line;

    read_char();
  }
}

std::string Lexer::read_ident()
{
  int start_pos = pos_;
  while (is_letter(ch_))
  {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

std::string Lexer::read_number()
{
  int start_pos = pos_;
  while (is_digit(ch_))
  {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

char Lexer::peek_char()
{
  if (read_pos_ >= (int)input_.size())
    return 0;
  else
    return input_[read_pos_];
}

LToken LLexer::next_token()
{
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

  switch (c)
  {
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
    if (match('>'))
    {
      return make_token(TokenType::Arrow);
    }
    else if (match('='))
    {
      return make_token(TokenType::Eq);
    }

    return make_token(TokenType::Assign);
  case '^':
    return make_token(TokenType::Xor);
  case '<':
    if (match('='))
    {
      return make_token(TokenType::ELT);
    }
    else if ('<')
    {
      return make_token(TokenType::LShift);
    }

    return make_token(TokenType::LT);
  case '>':
    if (match('='))
    {
      return make_token(TokenType::ELT);
    }
    else if ('>')
    {
      return make_token(TokenType::RShift);
    }

    return make_token(TokenType::GT);
  case '"':
    return str();
  }

  return error_token("Unexpected character.");
}

LToken LLexer::ident()
{
  while (std::isalnum(peek()))
    advance();
  return make_token(ident_type());
}

LToken LLexer::num()
{
  while (std::isdigit(peek()))
    advance();

  if (peek() == '.' && std::isdigit(peek_next()))
  {
    advance();
    while (std::isdigit(peek()))
      advance();
  }

  return make_token(TokenType::Int);
}

LToken LLexer::str()
{
  while (peek() != '"' && !is_at_end())
  {
    if (peek() == '\n')
      ++line_;
    advance();
  }

  if (is_at_end())
    return error_token("unterminated string");

  advance();
  return make_token(TokenType::String);
}

void LLexer::skip()
{
  while (true)
  {
    char c = peek();
    switch (c)
    {
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
      if (peek_next() == '/')
      {
        while (peek() != '\n' && !is_at_end())
          advance();
      }
      else
      {
        return;
      }
      break;
    default:
      return;
    }
  }
}

TokenType LLexer::ident_type() const
{
  switch (src.at(start_))
  {
  case 'e':
    return check_keyword(1, 3, "lse", TokenType::Else);
  case 'f':
    if (curr_ - start_ > 1)
    {
      switch (src.at(start_ + 1))
      {
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
    if (curr_ - start_ > 1)
    {
      switch (src.at(start_ + 1))
      {
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
    if (curr_ - start_ > 1)
    {
      switch (src.at(start_ + 1))
      {
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
                                TokenType type) const
{
  if (curr_ - start_ == begin + length &&
      src.substr(start_ + begin, length) == rest)
    return type;

  return TokenType::Ident;
}

LToken LLexer::error_token(std::string message) const noexcept
{
  return LToken(TokenType::Eof, message, line_);
}

LToken LLexer::make_token(TokenType type) const noexcept
{
  size_t length = curr_ - start_;
  return LToken(type, src.substr(start_, length), line_);
}

char LLexer::advance()
{
  ++curr_;
  return src.at(curr_ - 1);
}

bool LLexer::is_at_end() const noexcept { return curr_ == src.length(); }

bool LLexer::match(char expected)
{
  if (is_at_end())
    return false;
  if (src.at(curr_) != expected)
    return false;
  ++curr_;

  return true;
}

char LLexer::peek() const
{
  if (is_at_end())
    return '\0';
  return src.at(curr_);
}

char LLexer::peek_next() const
{
  if (is_at_end() || curr_ + 1 == src.length())
    return '\0';
  return src.at(curr_ + 1);
}
