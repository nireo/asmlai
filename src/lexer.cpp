#include "lexer.h"
#include "token.h"
#include <cctype>

Lexer::Lexer(const std::string &input)
{
  input_ = input;
  line = 0;
  read_pos_ = 0;
  read_char();
}

static bool
is_letter(char ch)
{
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool
is_digit(char ch)
{
  return '0' <= ch && ch <= '9';
}

void
Lexer::read_char()
{
  if(read_pos_ >= (int)input_.size())
    ch_ = 0;
  else
    ch_ = input_[read_pos_];

  pos_ = read_pos_;
  read_pos_++;
}

Token
new_token(tokentypes type, char ch)
{
  return Token{ type, std::string(1, ch) };
}

tokentypes
lookup_ident(const std::string &str)
{
  auto keyword = TokenKeywords.find(str);
  if(keyword == TokenKeywords.end())
    return tokentypes::Ident;
  return keyword->second;
}

Token
Lexer::next_token()
{
  Token tok;

  skip_whitespace();

  switch(ch_) {
  case '=':
    if(peek_char() == '=') {
      tok = Token{ .type = tokentypes::Eq, .literal = "==" };
      read_char();
    } else if(peek_char() == '>') {
      tok = Token{ .type = tokentypes::Arrow, .literal = "=>" };
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
    tok = new_token(tokentypes::Plus, ch_);
    read_char();
    break;
  case '-':
    tok = new_token(tokentypes::Minus, ch_);
    read_char();
    break;
  case '!':
    if(peek_char() == '=') {
      tok = Token{ .type = tokentypes::Neq, .literal = "!=" };
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
    if(peek_char() == '=') {
      tok = Token{ .type = tokentypes::ELT, .literal = "<=" };
      read_char();
    } else {
      tok = new_token(tokentypes::LT, ch_);
    }
    read_char();
    break;
  case '>':
    if(peek_char() == '=') {
      tok = Token{ .type = tokentypes::EGT, .literal = ">=" };
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
  case '&':
    tok = new_token(tokentypes::Amper, ch_);
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
    if(is_letter(ch_)) {
      tok.literal = read_ident();
      tok.type = lookup_ident(tok.literal);
    } else if(is_digit(ch_)) {
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

std::string
Lexer::read_string()
{
  int start_pos = pos_ + 1;
  for(;;) {
    read_char();
    if(ch_ == '"' || ch_ == 0) {
      break;
    }
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

void
Lexer::skip_whitespace()
{
  while(ch_ == ' ' || ch_ == '\t' || ch_ == '\n' || ch_ == '\r') {
    if (ch_ == '\n')
      ++line;

    read_char();
  }
}

std::string
Lexer::read_ident()
{
  int start_pos = pos_;
  while(is_letter(ch_)) {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

std::string
Lexer::read_number()
{
  int start_pos = pos_;
  while(is_digit(ch_)) {
    read_char();
  }

  return input_.substr(start_pos, pos_ - start_pos);
}

char
Lexer::peek_char()
{
  if(read_pos_ >= (int)input_.size())
    return 0;
  else
    return input_[read_pos_];
}
