#ifndef LAI_CODEX64_H
#define LAI_CODEX64_H

#include "compiler.h"
#include "token.h"
#include <string>

int get_bytesize_of_type(valuetype);
int load_into_register(int);
int mul_registers(int, int);
int div_registers(int, int);
int add_registers(int, int);
int sub_registers(int, int);
void print_register(int);
void init_out_file();
void gen_start();
void end_codegen();
int store_global(int, const Symbol &);
void generate_sym(const Symbol &);
int load_global(const Symbol &);
void free_all_registers();
int codegen_compare_no_jump(int, int, const tokentypes);
int codegen_compare_jump(int, int, int, const tokentypes);
void gen_jmp(int);
void gen_label(int);
void function_start(const std::string &);
void function_end(int);
int codegen_call(int, const std::string &);
void codegen_return(int, const Symbol &);
int codegen_addr(const Symbol &);
int codegen_dereference(int, const valuetype);
int shift_left(int, int);
int codegen_load_int(int);
int store_dereference(int, int, valuetype);
void global_str(int, std::string);
int load_global_str(int);

#endif
