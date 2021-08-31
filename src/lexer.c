#include "lexer.h"
#include "string.h"

Lexer lex;

static int
match(char exp)
{
  if(*lex.current == '\0')
    return 0;

  if(*lex.current != exp)
    return 0;

  ++lex.current;
  return 1;
}

void
init_lexer(const char *source)
{
  lex.start = source;
  lex.current = source;
  lex.line = 1;
}

static char
next()
{
  // take last character and return it
  ++lex.current;
  return lex.current[-1];
}

static Token
take_token(TokenType type)
{
  Token tok;
  tok.type = type;
  tok.start = lex.start;
  tok.len = (int)(lex.current - lex.start);
  tok.line = lex.line;

  return tok;
}

static Token
err_token(const char *err_msg)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.start = err_msg;
  token.len = (int)strlen(err_msg);
  token.line = lex.line;

  return token;
}

static char
peek()
{
  return *lex.current;
}
static char
peek_next()
{
  if(*lex.current == '\0')
    return '\0';

  return lex.current[1];
}

static void
skip_unwanted()
{
  for(;;) {
    char ch = peek();

    switch(ch) {
    case ' ':
    case '\r':
    case '\t':
      next();
      break;
    case '/':
      if(peek_next() == '/') {
        // A comment goes until the end of the line.
        while(peek() != '\n' && !(*lex.current == '\0'))
          next();
      } else {
        return;
      }
      break;
    case '\n':
      ++lex.line;
      next();
      break;
    default:
      return;
    }
  }
}

static int
is_number(char c)
{
  return c >= '0' && c <= '9';
}
static int
is_alpha(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType
is_keyword(int start, int len, const char *rest, TokenType type)
{
  if(lex.current - lex.start == start + len
     && memcmp(lex.start + start, rest, len) == 0)
    return type;

  return TOKEN_IDENTIFIER;
}

static Token
num()
{
  while(is_number(peek()))
    next();

  if(peek() == '.' && is_number(peek_next())) {
    next();
    while(*lex.current == '\0')
      next();
  }

  return take_token(TOKEN_NUMBER);
}

static Token
string()
{
  while(peek() != '"' && !(*lex.current == '\0')) {
    if(peek() == '\n')
      ++lex.line;
    next();
  }

  if(*lex.current == '\0')
    return err_token("string doesn't end");

  next();

  return take_token(TOKEN_STRING);
}

static TokenType
get_identifier_type()
{
  switch(lex.start[0]) {
  case 'c':
    if(lex.current - lex.start > 1) {
      switch(lex.start[1]) {
      case 'o':
        return is_keyword(2, 6, "ntinue", TOKEN_CONTINUE);
      case 'a':
        return is_keyword(2, 2, "se", TOKEN_CASE);
      }
    }
    break;
  case 's':
    if(lex.current - lex.start > 1) {
      switch(lex.start[1]) {
      case 'h':
        return is_keyword(2, 3, "ort", TOKEN_SHORT);
      case 'w':
        return is_keyword(2, 4, "itch", TOKEN_SWITCH);
      case 'i':
        return is_keyword(2, 4, "gned", TOKEN_SIGNED);
      case 't':
        if(lex.current - lex.start > 2) {
          switch(lex.start[2]) {
          case 'a':
            return is_keyword(3, 3, "tic", TOKEN_STATIC);
          case 'r':
            return is_keyword(2, 3, "uct", TOKEN_STRUCT);
          }
        }
        break;
      }
    case 'd':
      if(lex.current - lex.start > 1) {
        switch(lex.start[1]) {
        case 'e':
          return is_keyword(2, 5, "fault", TOKEN_DEFAULT);
        case 'o':
          if(lex.current - lex.start > 2) {
            switch(lex.start[2]) {
            case 'u':
              return is_keyword(3, 3, "ble", TOKEN_DOUBLE);
            case 'o':
              return TOKEN_DO;
            }
          }
        }
        break;
      }
    case 'b':
      return is_keyword(1, 4, "reak", TOKEN_BREAK);
    case 't':
      return is_keyword(1, 6, "ypedef", TOKEN_TYPEDEF);
    case 'v':
      return is_keyword(1, 3, "oid", TOKEN_VOID);
    case 'r':
      return is_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 'e':
      return is_keyword(1, 3, "num", TOKEN_ENUM);
    case 'p':
      return is_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'i': {
      if(lex.current - lex.start > 1) {
        switch(lex.start[1]) {
        case 'n':
          return is_keyword(2, 1, "t", TOKEN_INT);
        case 'f':
          return TOKEN_IF;
        }
      }
      break;
    }
    }
    break;
  }

  return TOKEN_IDENTIFIER;
}

static Token
identifier()
{
  while(is_alpha(peek()) || is_number(peek()))
    next();

  return take_token(get_identifier_type());
}

Token
get_token()
{
  skip_unwanted();

  lex.start = lex.current;
  if(*lex.current == '\0')
    return take_token(TOKEN_EOF);

  char c = next();

  if(is_alpha(c))
    return identifier();
  if(is_number(c))
    return num();

  switch(c) {
  case '(':
    return take_token(TOKEN_LEFT_PAREN);
  case ')':
    return take_token(TOKEN_RIGHT_PAREN);
  case '{':
    return take_token(TOKEN_LEFT_BRACE);
  case '}':
    return take_token(TOKEN_RIGHT_BRACE);
  case '[':
    return take_token(TOKEN_LEFT_BRACKET);
  case '%':
    return take_token(TOKEN_MODULO);
  case ']':
    return take_token(TOKEN_RIGHT_BRACKET);
  case ';':
    return take_token(TOKEN_SEMICOLON);
  case ':':
    return take_token(TOKEN_COLON);
  case ',':
    return take_token(TOKEN_COMMA);
  case '.':
    return take_token(TOKEN_DOT);
  case '-':
    if(match('=')) {
      return take_token(TOKEN_MINUS_EQUAL);
    }
    return take_token(TOKEN_MINUS);
  case '+':
    if(match('=')) {
      return take_token(TOKEN_PLUS_EQUAL);
    }
    return take_token(TOKEN_PLUS);
  case '/':
    if(match('=')) {
      return take_token(TOKEN_SLASH_EQUAL);
    }
    return take_token(TOKEN_SLASH);
  case '*':
    if(match('=')) {
      return take_token(TOKEN_STAR_EQUAL);
    }
    return take_token(TOKEN_STAR);
  case '!':
    return take_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return take_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    if(match('=')) {
      return take_token(TOKEN_LESS_EQUAL);
    } else if(match('<')) {
      return take_token(TOKEN_BITLEFT);
    }
    return take_token(TOKEN_LESS);
  case '>':
    if(match('=')) {
      return take_token(TOKEN_GREATER_EQUAL);
    } else if(match('>')) {
      return take_token(TOKEN_BITRIGHT);
    }
    return take_token(TOKEN_GREATER);
  case '~':
    return take_token(TOKEN_COMPLEMENT);
  case '&':
    return take_token(match('&') ? TOKEN_AND : TOKEN_BITAND);
  case '|':
    return take_token(match('|') ? TOKEN_OR : TOKEN_BITOR);
  case '"':
    return string();
  }

  return err_token("unexpected token");
}
