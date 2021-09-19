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
  ELT,
  EGT,
  Function,
  Let,
  True,
  False,
  If,
  Else,
  Return,
  String,
  Print,
  While,
  For,
  Arrow,

  IntType,
  CharType,
  StringType,
  FloatType,
  Void,
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
  { "float", tokentypes::FloatType }, { "print", tokentypes::Print },
  { "while", tokentypes::While },     { "for", tokentypes::For },
  { "void", tokentypes::Void },
};

#endif
