#ifndef LAI_TOKEN_H
#define LAI_TOKEN_H

#include <string>
#include <unordered_map>

enum class TokenType {
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
  Amper,

  LogOr,
  LogAnd,
  LogNot,
  Or,
  Xor,
  LShift,
  RShift,
  Inc,
  Dec,
  Invert,

  IntType,
  CharType,
  StringType,
  LongType,
  FloatType,
  Void,
  Global,
  Var,
};

struct Token {
  TokenType type;
  std::string literal;
};

const std::unordered_map<std::string, TokenType> TokenKeywords = {
    {"fn", TokenType::Function},     {"let", TokenType::Let},
    {"true", TokenType::True},       {"false", TokenType::False},
    {"if", TokenType::If},           {"else", TokenType::Else},
    {"return", TokenType::Return},   {"int", TokenType::IntType},
    {"char", TokenType::CharType},   {"string", TokenType::StringType},
    {"float", TokenType::FloatType}, {"print", TokenType::Print},
    {"while", TokenType::While},     {"for", TokenType::For},
    {"void", TokenType::Void},       {"global", TokenType::Global},
    {"var", TokenType::Var}};

#endif
