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
  int label;
};

int compile_ast_node(const Node &, int, const AstType);
const Symbol &get_symbol(const std::string &);
Symbol &get_symbol_ref(const std::string &);
void add_new_symbol(const std::string &, const symboltype, const valuetype);
void add_new_symbol(const std::string &, const symboltype, const valuetype, int label);
bool symbol_exists(const std::string &);
bool check_type_compatible(const valuetype, const valuetype, bool noleft);
int get_next_label();

#endif
