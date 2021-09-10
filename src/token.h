#ifndef LAI_TOKEN_H
#define LAI_TOKEN_H

#include <string>
#include <unordered_map>

typedef std::string TokenType;
struct Token {
  TokenType type;
  std::string literal;
};

namespace tokentypes
{
const TokenType ILLEGAL = "ILLEGAL";
const TokenType EOFF = "EOF";
const TokenType IDENT = "IDENT";
const TokenType INT = "INT";

const TokenType ASSIGN = "=";
const TokenType PLUS = "+";
const TokenType MINUS = "=";
const TokenType BANG = "!";
const TokenType ASTERISK = "*";
const TokenType SLASH = "/";
const TokenType COMMA = ",";
const TokenType SEMICOLON = ";";
const TokenType EQ = "==";
const TokenType NEQ = "!=";
const TokenType COLON = ":";

const TokenType LPAREN = "(";
const TokenType RPAREN = ")";
const TokenType LBRACE = "{";
const TokenType RBRACE = "}";
const TokenType LBRACKET = "[";
const TokenType RBRACKET = "]";

const TokenType LT = "<";
const TokenType GT = ">";

const TokenType FUNCTION = "FUNCTION";
const TokenType LET = "LET";
const TokenType TRUE = "TRUE";
const TokenType FALSE = "FALSE";
const TokenType IF = "IF";
const TokenType ELSE = "ELSE";
const TokenType RETURN = "RETURN";
const TokenType STRING = "STRING";

const std::unordered_map<std::string, TokenType> KEYWORDS = {
  { "func", FUNCTION }, { "let", LET }, { "true", TRUE },
  { "false", FALSE },   { "if", IF },   { "else", ELSE },
  { "return", RETURN },
};
} // namespace tokentypes

#endif
