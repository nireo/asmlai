#ifndef LAI_PARSER_H
#define LAI_PARSER_H

#include "lexer.h"

typedef struct {
  Token current;
  Token previous;
  int had_error;
  int panic_mode;
} Parser;

#endif
