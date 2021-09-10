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

  IntType,
  CharType,
  StringType,
  FloatType,
};

struct Token {
  tokentypes type;
  std::string literal;
};

const std::unordered_map<std::string, tokentypes> TokenKeywords = {
  { "fn", tokentypes::Function },     { "let", tokentypes::Let },
  { "true", tokentypes::True },       { "false", tokentypes::False },
  { "if", tokentypes::If },           { "else", tokentypes::Else },
  { "return", tokentypes::Return },   { "int", tokentypes::IntType },
  { "char", tokentypes::CharType },   { "string", tokentypes::StringType },
  { "float", tokentypes::FloatType },
};

#endif
