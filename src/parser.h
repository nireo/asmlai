#ifndef LAI_PARSER_H
#define LAI_PARSER_H

#include "ast.h"
#include "lexer.h"
#include <memory>
#include <unordered_map>
#include <vector>

class Parser;
typedef std::unique_ptr<Expression> (*PrefixParseFn)();
typedef std::unique_ptr<Expression> (*InfixParseFn)(
    std::unique_ptr<Expression>);

enum Precedence {
  LOWEST,
  EQUALS,
  LESSGREATER,
  SUM,
  PRODUCT,
  PREFIX,
  CALL,
  INDEX,
};

std::vector<std::unique_ptr<Statement> > parse(const char *source);

#endif
