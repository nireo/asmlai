#ifndef LAI_COMPILER_H
#define LAI_COMPILER_H

#include "ast.h"
#include "token.h"

enum class Scope {
  Global,
  Local,
  Parameter,
};

enum symboltype {
  TYPE_VARIABLE,
  TYPE_FUNCTION,
  TYPE_ARRAY,
};

struct Symbol {
  std::string name_;
  Scope st_type;
  symboltype type_;
  valuetype value_type_;
  int label;
  int size;
  int position;
};

void add_new_local_var(const std::string &, valuetype, int, int);
void reset_local_variables();
void add_new_param(const std::string &, valuetype, int, int);
int compile_ast_node(const Node &, int, const AstType);
const Symbol &get_symbol(const std::string &);
Symbol &get_symbol_ref(const std::string &);
void add_new_symbol(const std::string &, const symboltype, const valuetype);
void add_new_symbol(const std::string &, const symboltype, const valuetype,
                    int);
void add_new_symbol(const std::string &, const symboltype, const valuetype, int,
                    int);
bool symbol_exists(const std::string &);
bool check_type_compatible(const valuetype, const valuetype, bool noleft);
std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>
    change_type(std::unique_ptr<Expression>, valuetype, TokenType);
int get_next_label();

#endif
