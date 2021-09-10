#ifndef LAI_TOKEN_H
#define LAI_TOKEN_H

#include <string>
#include <unordered_map>

enum class tokentypes {
  Illegal,
  Eof,
  Ident,
  Int,
  Assign,
  Plus,
  Minus,
  Bang,
  Asterisk,
  Slash,
  Comma,
  Semicolon,
  Eq,
  Neq,
  Colon,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  LT,
  GT,
  Function,
  Let,
  True,
  False,
  If,
  Else,
  Return,
  String,
};

struct Token {
  tokentypes type;
  std::string literal;
};

const std::unordered_map<std::string, tokentypes> TokenKeywords = {
  { "func", tokentypes::Function }, { "let", tokentypes::Let },
  { "true", tokentypes::True },     { "false", tokentypes::False },
  { "if", tokentypes::If },         { "else", tokentypes::Else },
  { "return", tokentypes::Return },
};

#endif
