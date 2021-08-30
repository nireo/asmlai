#include <stdio.h>
#include <stdlib.h>

static FILE *out;
static int free_registers[4];
static char *registers[4] = {"%r8", "%r9", "%r10", "%r11"};

static void free_all(void) {
  free_registers[0] = 1;
  free_registers[1] = 1;
  free_registers[2] = 1;
  free_registers[3] = 1;
}

static int get_register(void) {
  for (int i = 0; i < 4; ++i) {
    if (free_registers[i]) {
      free_registers[i] = 0;
      return i;
    }
  }

  fprintf(stderr, "no free registers\n");
  exit(1);
}

void add_code_gen_start() {
  free_all();

  fputs("\t.file \"main.c\"\n"
        "\t.text\n"
        ".LC0:\n"
        "\t.string\t\"%d\\n\"\n"
        "printint:\n"
        "\tpushq\t%rbp\n"
        "\tmovq\t%rsp, %rbp\n"
        "\tsubq\t$16, %rsp\n"
        "\tmovl\t%edi, -4(%rbp)\n"
        "\tmovl\t-4(%rbp), %eax\n"
        "\tmovl\t%eax, %esi\n"
        "\tleaq	.LC0(%rip), %rdi\n"
        "\tmovl	$0, %eax\n"
        "\tcall	printf@PLT\n"
        "\tnop\n"
        "\tleave\n"
        "\tret\n"
        "\n"
        "\t.globl\tmain\n"
        "\t.type\tmain, @function\n"
        "main:\n"
        "\tpushq %rbp\n"
        "\tmovq %rsp, %rbp\n",
        out);
}

void return_zero_code_gen() {
  fputs("\tmovl $0, %eax\n"
        "\tpopq %rbp\n"
        "\tret\n",
        out);
}

static void free_register(int reg) {
  if (free_registers[reg] != 0) {
    fprintf(stderr, "error trying to free register\n");
    exit(1);
  }

  free_registers[reg] = 1;
}

int add_registers(int reg1, int reg2) {
  fprintf(out, "\taddq\t%s, %s\n", registers[reg1], registers[reg2]);
  free_register(reg1);
  return reg2;
}

int sub_registers(int reg1, int reg2) {
  fprintf(out, "\tsubq\t%s, %s\n", registers[reg2], registers[reg1]);
  free_register(reg2);

  return reg1;
}

void init_out_file() {
  if ((out = fopen("out.s", "w")) == NULL) {
    fprintf(stderr, "unable to create out file.\n");
    exit(1);
  }
}

int load_into_register(int val) {
  int reg = get_register();
  fprintf(out, "\tmovq\t$%d, %s\n", val, registers[reg]);

  return reg;
}


int mul_registers(int r1, int r2) {
  fprintf(out, "\timulq\t%s, %s\n", registers[r1], registers[r2]);
  free_register(r1);
  return r2;
}

int div_registers(int r1, int r2) {
  fprintf(out, "\tmovq\t%s,%%rax\n\tcqo\n", registers[r1]);
  fprintf(out, "\tidivq\t%s\n", registers[r2]);
  fprintf(out, "\tmovq\t%%rax,%s\n", registers[r1]);
  free_register(r2);

  return r1;
}


void print_register(int r) {
  fprintf(out, "\tmovq\t%s, %%rdi\n"
               "\tcall\tprint_integer\n", registers[r]);
  free_register(r);
}
