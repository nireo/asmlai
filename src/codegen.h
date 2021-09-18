#ifndef LAI_CODEGEN_H
#define LAI_CODEGEN_H

#include "token.h"
#include <string>

int load_into_register(int);
int mul_registers(int, int);
int div_registers(int, int);
int add_registers(int, int);
int sub_registers(int, int);
void print_register(int);
void init_out_file();
void gen_start();
void end_codegen();
int store_global(int, std::string);
void generate_sym(std::string);
int load_global(std::string);
void free_all_registers();
int codegen_compare_no_jump(int, int, const tokentypes);
int codegen_compare_jump(int, int, int, const tokentypes);
void gen_jmp(int);
void gen_label(int);
void function_start(const std::string&);
void function_end();

#endif
