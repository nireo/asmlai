#ifndef LAI_CODEGEN_H
#define LAI_CODEGEN_H

int load_into_register(int);
int mul_registers(int, int);
int div_registers(int, int);
int add_registers(int, int);
int sub_registers(int, int);
void print_register(int);
void init_out_file();
void gen_start();
void end_codegen();

#endif
