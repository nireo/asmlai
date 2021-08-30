#ifndef LAI_CODE_GEN_H
#define LAI_CODE_GEN_H

int load_into_register(int val);
int add_registers(int reg1, int reg2);
int mul_registers(int reg1, int reg2);
int sub_registers(int reg1, int reg2);
int div_registers(int reg1, int reg2);

void init_out_file();
void return_zero_code_gen();
void add_code_gen_start();
void print_register(int r);

#endif
