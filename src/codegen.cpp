#include "codegen.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static std::FILE *fp = nullptr;
static bool free_registers[4];
static const std::string registers[4] = { "%r8", "%r9", "%r10", "%r11" };
static const std::string b_registers[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };

static const std::string jump_insts[]
    = { "jne", "je", "jge", "jle", "jg", "jl" };
static const std::string compare_instructions[]
    = { "sete", "setne", "setl", "setg", "setle", "setge" };

static int
get_corresponding_inst_index(const tokentypes type)
{
  switch(type) {
  case tokentypes::Eq:
    return 0;
  case tokentypes::Neq:
    return 1;
  case tokentypes::LT:
    return 2;
  case tokentypes::GT:
    return 3;
  case tokentypes::ELT:
    return 4;
  case tokentypes::EGT:
    return 5;
  default:
    std::fprintf(stderr, "no free registers\n");
    std::exit(1);
  }
}

void
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

int
store_global(int r, std::string identifier)
{
  std::fprintf(fp, "\tmovq\t%s, %s(\%%rip)\n", registers[r].c_str(),
               identifier.c_str());
  return r;
}

void
generate_sym(std::string symbol)
{
  std::fprintf(fp, "\t.comm\t%s,8,8\n", symbol.c_str());
}

int
load_global(std::string identifier)
{
  int free_reg = get_register();

  std::fprintf(fp, "\tmovq\t%s(\%%rip), %s\n", identifier.c_str(),
               registers[free_reg].c_str());
  return free_reg;
}

int
codegen_compare_no_jump(int reg1, int reg2, const tokentypes type)
{
  auto index = get_corresponding_inst_index(type);

  std::fprintf(fp, "\tcmpq\t%s, %s\n", registers[reg2].c_str(),
               registers[reg1].c_str());
  std::fprintf(fp, "\t%s\t%s\n", compare_instructions[index].c_str(),
               b_registers[reg2].c_str());
  std::fprintf(fp, "\tmovzbq\t%s, %s\n", b_registers[reg2].c_str(),
               registers[reg2].c_str());

  free_register(reg1);

  return reg2;
}

int
codegen_compare_jump(int reg1, int reg2, int label, const tokentypes type)
{
  auto index = get_corresponding_inst_index(type);

  std::fprintf(fp, "\tcmpq\t%s, %s\n", registers[reg2].c_str(), registers[reg1].c_str());
  std::fprintf(fp, "\t%s\tL%d\n", jump_insts[index].c_str(), label);
  free_all_registers();

  return -1;
}

void
gen_label(int label)
{
  std::fprintf(fp, "L%d:\n", label);
}

void
gen_jmp(int label)
{
  std::fprintf(fp, "\tjmp\tL%d\n", label);
}
