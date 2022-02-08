#ifndef LAI_CODEX64_H
#define LAI_CODEX64_H

#include "compiler.h"
#include "token.h"
#include <string>

namespace codegen {
int get_bytesize_of_type(ValueT);
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
int load_global(const Symbol &, TokenType, bool);
void free_all_registers();
int codegen_compare_no_jump(int, int, const TokenType);
int codegen_compare_jump(int, int, int, const TokenType);
void gen_jmp(int);
void gen_label(int);
void function_start(const std::string &);
void function_end(int);
int codegen_call(const std::string &, int);
void codegen_return(int, const Symbol &);
int codegen_addr(const Symbol &);
int codegen_dereference(int, const ValueT);
int shift_left(int, int);
int codegen_load_int(int);
int store_dereference(int, int, ValueT);
void global_str(int, const std::string &);
int load_global_str(int);
int shift_right_from_reg(int, int);
int shift_left_from_reg(int, int);
int codegen_and(int, int);
int codegen_or(int, int);
int codegen_xor(int, int);
int codegen_invert(int);
int codegen_neg(int);
int codegen_not(int);
void reset_local_offset();
int get_local_offset(ValueT);
int load_local(const Symbol &, TokenType, bool);
int store_local(const Symbol &, int);
void copy_argument(int, int);
}; // namespace codegen

#endif
