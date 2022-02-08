#include "symbol_table.h"
#include "codegen_x64.h"
#include <string>
#include <unordered_map>

std::unordered_map<std::string,
                   std::unordered_map<std::string, symtable::Symbol>>
    function_locals;
std::unordered_map<std::string, symtable::Symbol> global_symbols;

void symtable::create_new_function_table(const std::string &func_name) {
  function_locals[func_name] = std::unordered_map<std::string, Symbol>();
}

void symtable::new_function_local(const std::string &func_name,
                                  const std::string &name, ValueT vtype,
                                  int label, int size) {
  function_locals[func_name][name] = {
      .name_ = name,
      .st_type = Scope::Local,
      .type_ = TYPE_VARIABLE, // cannot be function
      .value_type_ = vtype,
      .label = label,
      .size = size,
      .position = 0,
  };
}

void symtable::new_function_param(const std::string &func_name,
                                  const std::string &name, ValueT vtype,
                                  int label, int size) {
  function_locals[func_name][name] = {
      .name_ = name,
      .st_type = Scope::Parameter,
      .type_ = TYPE_VARIABLE, // cannot be function
      .value_type_ = vtype,
      .label = label,
      .size = size,
      .position = 0,
  };
}

std::unordered_map<std::string, symtable::Symbol> &
symtable::get_function_locals(const std::string &name) {
  return function_locals[name];
}

void reset_local_variables() { codegen::reset_local_offset(); }

void symtable::add_new_symbol(const std::string &name, const symboltype stype,
                              const ValueT vtype) {
  global_symbols[name] = Symbol{
      .name_ = name,
      .type_ = stype,
      .value_type_ = vtype,
      .label = 0,
  };
}

void symtable::add_new_symbol(const std::string &name, const symboltype stype,
                              const ValueT vtype, int label) {
  global_symbols[name] = Symbol{
      .name_ = name,
      .st_type = Scope::Global,
      .type_ = stype,
      .value_type_ = vtype,
      .label = label,
      .position = 0,
  };
}

void symtable::add_new_symbol(const std::string &name, const symboltype stype,
                    const ValueT vtype, int label, int size) {
  global_symbols[name] = Symbol{.name_ = name,
                                .type_ = stype,
                                .value_type_ = vtype,
                                .label = label,
                                .size = size};
}

const symtable::Symbol &symtable::get_symbol(const std::string &name,
                                             const std::string &curr) {
  if (curr != "") {
    if (function_locals[curr].find(name) != function_locals[curr].end())
      return function_locals[curr][name];
  }

  if (global_symbols.find(name) != global_symbols.end()) {
    return global_symbols[name];
  }

  std::fprintf(stderr, "symbol with name '%s' not found\n", name.c_str());
  std::exit(1);
}

const symtable::Symbol &symtable::get_symbol_w_func(const std::string &func,
                                                    const std::string &name) {
  if (function_locals[func].find(name) != function_locals[func].end())
    return function_locals[func][name];

  if (global_symbols.find(name) != global_symbols.end()) {
    return global_symbols[name];
  }

  std::fprintf(stderr, "symbol with name '%s' not found\n", name.c_str());
  std::exit(1);
}

symtable::Symbol &symtable::get_symbol_ref(const std::string &name) {
  auto &symbol = global_symbols[name];
  return symbol;
}
