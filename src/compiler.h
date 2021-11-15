#ifndef LAI_COMPILER_H
#define LAI_COMPILER_H

#include "ast.h"
#include "token.h"
#include <string>
#include <unordered_map>

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
  ValueT value_type_;
  int label;
  int size;
  int position;
};

typedef std::unordered_map<std::string, Symbol> SymbolTable;
void create_new_function_table(const std::string &);
SymbolTable &get_function_locals(const std::string &);
void new_function_param(const std::string &, const std::string &, ValueT, int,
                        int);
void new_function_local(const std::string &, const std::string &, ValueT, int,
                        int);
void reset_local_variables();
void add_new_param(const std::string &, ValueT, int, int);
int compile_ast_node(const Node &, int, const AstType);
const Symbol &get_symbol(const std::string &);
const Symbol &get_symbol_w_func(const std::string &, const std::string &);
Symbol &get_symbol_ref(const std::string &);
void add_new_symbol(const std::string &, const symboltype, const ValueT);
void add_new_symbol(const std::string &, const symboltype, const ValueT, int);
void add_new_symbol(const std::string &, const symboltype, const ValueT, int,
                    int);
bool symbol_exists(const std::string &);
std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>
    change_type(std::unique_ptr<Expression>, ValueT, TokenType);
int get_next_label();

#endif
