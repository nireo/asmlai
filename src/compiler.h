#ifndef LAI_COMPILER_H
#define LAI_COMPILER_H

#include "ast.h"

enum symboltype {
  TYPE_VARIABLE,
  TYPE_FUNCTION,
};

struct Symbol {
  std::string name_;
  symboltype type_;
  valuetype value_type_;
};

int compile_ast_node(const Node &, int, const AstType);
const Symbol &get_symbol(const std::string &);
void add_new_symbol(const std::string &, const symboltype, const valuetype);

#endif
