#include "codegen.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static std::FILE *fp = nullptr;
static bool free_registers[4];
static std::string registers[4] = { "%r8", "%r9", "%r10", "%r11" };

static void
free_all_registers()
{
  free_registers[0] = true;
  free_registers[1] = true;
  free_registers[2] = true;
  free_registers[3] = true;
}

static int
get_register()
{
  for(int i = 0; i < 4; ++i) {
    if(free_registers[i]) {
      free_registers[i] = false;
      return i;
    }
  }

  std::fprintf(stderr, "no free registers\n");
  std::exit(1);
}

static void
free_register(int reg)
{
  if(free_registers[reg] != 0) {
    std::fprintf(stderr, "register is already free");
    std::exit(1);
  }

  free_registers[reg] = true;
}

void
init_out_file()
{
  if((fp = fopen("out.s", "w")) == NULL) {
    std::fprintf(stderr, "unable to create out file\n");
    std::exit(1);
  }
}

void
gen_start()
{
  free_all_registers();

  fputs("\t.file \"main.c\"\n"
        "\t.text\n"
        ".LC0:\n"
        "\t.string\t\"%d\\n\"\n"
        "test_print_integer:\n"
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
        fp);
}

int
load_into_register(int val)
{
  int reg = get_register();
  fprintf(fp, "\tmovq\t$%d, %s\n", val, registers[reg].c_str());

  return reg;
}

int
mul_registers(int r1, int r2)
{
  fprintf(fp, "\timulq\t%s, %s\n", registers[r1].c_str(),
          registers[r2].c_str());
  free_register(r1);

  return r2;
}

int
div_registers(int r1, int r2)
{
  fprintf(fp, "\tmovq\t%s,%%rax\n\tcqo\n", registers[r1].c_str());
  fprintf(fp, "\tidivq\t%s\n", registers[r2].c_str());
  fprintf(fp, "\tmovq\t%%rax,%s\n", registers[r1].c_str());

  free_register(r2);

  return r1;
}

int
add_registers(int reg1, int reg2)
{
  fprintf(fp, "\taddq\t%s, %s\n", registers[reg1].c_str(),
          registers[reg2].c_str());
  free_register(reg1);
  return reg2;
}

int
sub_registers(int reg1, int reg2)
{
  fprintf(fp, "\tsubq\t%s, %s\n", registers[reg2].c_str(),
          registers[reg1].c_str());
  free_register(reg2);

  return reg1;
}

void
print_register(int r)
{
  fprintf(fp,
          "\tmovq\t%s, %%rdi\n"
          "\tcall\ttest_print_integer\n",
          registers[r].c_str());
  free_register(r);
}

void
end_codegen()
{
  fputs("\tmovl $0, %eax\n"
        "\tpopq %rbp\n"
        "\tret\n",
        fp);
  fclose(fp);
}
