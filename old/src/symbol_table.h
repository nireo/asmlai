#ifndef LAI_SYMTABLE_H
#define LAI_SYMTABLE_H

#include "ast.h"
#include <string>

namespace symtable {
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

void add_new_symbol(const std::string &, const symboltype, const ValueT);
void add_new_symbol(const std::string &, const symboltype, const ValueT, int);
void add_new_symbol(const std::string &, const symboltype, const ValueT, int,
                    int);

void create_new_function_table(const std::string &);
std::unordered_map<std::string, Symbol> &
get_function_locals(const std::string &);
void new_function_param(const std::string &, const std::string &, ValueT, int,
                        int);
void new_function_local(const std::string &, const std::string &, ValueT, int,
                        int);
void add_new_param(const std::string &, ValueT, int, int);
const Symbol &get_symbol(const std::string &, const std::string &);
const Symbol &get_symbol_w_func(const std::string &, const std::string &);
Symbol &get_symbol_ref(const std::string &);
}; // namespace symtable

#endif
