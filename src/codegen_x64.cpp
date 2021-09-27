#include "codegen_x64.h"
#include "ast.h"
#include "compiler.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static std::FILE *fp = nullptr;
static bool free_registers[4];
static const std::string registers[4] = { "%r8", "%r9", "%r10", "%r11" };
static const std::string b_registers[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };
static const std::string d_registers[4] = { "%r8d", "%r9d", "%r10d", "%r11d" };

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

int
get_bytesize_of_type(valuetype type)
{
  switch(type) {
  case TYPE_VOID:
    return 0;
  case TYPE_CHAR:
    return 1;
  case TYPE_INT:
    return 4;
  case TYPE_PTR_CHAR:
  case TYPE_PTR_VOID:
  case TYPE_PTR_INT:
  case TYPE_PTR_LONG:
  case TYPE_LONG:
    return 8;
  default: {
    std::fprintf(stderr, "type not found.");
    std::exit(1);
  }
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

  fputs("\t.text\n"
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
        "\tret\n",
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
store_global(int r, const Symbol &sym)
{
  switch(sym.value_type_) {
  case TYPE_CHAR: {
    std::fprintf(fp, "\tmovb\t%s, %s(\%%rip)\n", b_registers[r].c_str(),
                 sym.name_.c_str());
    break;
  }
  case TYPE_INT: {
    std::fprintf(fp, "\tmovl\t%s, %s(\%%rip)\n", d_registers[r].c_str(),
                 sym.name_.c_str());
    break;
  }
  case TYPE_LONG: {
    std::fprintf(fp, "\tmovq\t%s, %s(\%%rip)\n", registers[r].c_str(),
                 sym.name_.c_str());
    break;
  }
  case TYPE_PTR_CHAR:
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    std::fprintf(fp, "\tmovq\t%s, %s(\%%rip)\n", registers[r].c_str(),
                 sym.name_.c_str());
    break;
  }
  default: {
    std::fprintf(stderr, "cannot load global type.\n");
    std::exit(1);
  }
  }

  return r;
}

void
generate_sym(const Symbol &sym)
{
  int size = get_bytesize_of_type(sym.value_type_);

  std::fprintf(fp,
               "\t.data\n"
               "\t.globl\t%s\n",
               sym.name_.c_str());

  switch(size) {
  case 1:
    std::fprintf(fp, "%s:\t.byte\t0\n", sym.name_.c_str());
    break;
  case 4:
    std::fprintf(fp, "%s:\t.long\t0\n", sym.name_.c_str());
    break;
  case 8:
    std::fprintf(fp, "%s:\t.quad\t0\n", sym.name_.c_str());
    break;
  default:
    std::fprintf(stderr, "unrecognized byte size.");
    std::exit(1);
  }
}

int
load_global(const Symbol &sym)
{
  int free_reg = get_register();

  switch(sym.value_type_) {
  case TYPE_CHAR: {
    std::fprintf(fp, "\tmovzbq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg].c_str());
    break;
  }
  case TYPE_INT: {
    std::fprintf(fp, "\tmovzbq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg].c_str());
    break;
  }
  case TYPE_LONG: {
    std::fprintf(fp, "\tmovq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg].c_str());
    break;
  }
  case TYPE_PTR_CHAR:
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    std::fprintf(fp, "\tmovq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg].c_str());
    break;
  }
  default: {
    std::fprintf(stderr, "cannot load global type.\n");
    std::exit(1);
  }
  }

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

  std::fprintf(fp, "\tcmpq\t%s, %s\n", registers[reg2].c_str(),
               registers[reg1].c_str());
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

void
function_start(const std::string &name)
{
  std::fprintf(fp,
               "\t.text\n"
               "\t.globl\t%s\n"
               "\t.type\t%s, @function\n"
               "%s:\n"
               "\tpushq\t%%rbp\n"
               "\tmovq\t%%rsp, %%rbp\n",
               name.c_str(), name.c_str(), name.c_str());
}

void
codegen_return(int reg, const Symbol &sym)
{
  switch(sym.value_type_) {
  case TYPE_CHAR:
    fprintf(fp, "\tmovzbl\t%s, %%eax\n", b_registers[reg].c_str());
    break;
  case TYPE_INT:
    fprintf(fp, "\tmovl\t%s, %%eax\n", d_registers[reg].c_str());
    break;
  case TYPE_LONG:
    std::fprintf(fp, "\tmovq\t%s, %%rax\n", registers[reg].c_str());
    break;
  default: {
    std::fprintf(stderr, "undefined return function");
    std::exit(1);
  }
  }

  gen_jmp(sym.label);
}

int
codegen_call(int reg, const std::string &name)
{
  int outer = get_register();
  std::fprintf(fp, "\tmovq\t%s, %%rdi\n", registers[reg].c_str());
  std::fprintf(fp, "\tcall\t%s\n", name.c_str());
  std::fprintf(fp, "\tmovq\t%%rax, %s\n", registers[outer].c_str());

  free_register(reg);

  return outer;
}

void
function_end(int label)
{
  gen_label(label);
  std::fputs("\tpopq	%rbp\n"
             "\tret\n",
             fp);
}

int
codegen_addr(const Symbol &sym)
{
  int r = get_register();

  std::fprintf(fp, "\tleaq\t%s(%%rip), %s\n", sym.name_.c_str(),
               registers[r].c_str());

  return r;
}

int
codegen_dereference(int reg, const valuetype type)
{
  switch(type) {
  case TYPE_PTR_CHAR: {
    std::fprintf(fp, "\tmovzbq\t(%s), %s\n", registers[reg].c_str(),
                 registers[reg].c_str());
    break;
  }
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    std::fprintf(fp, "\tmovq\t(%s), %s\n", registers[reg].c_str(),
                 registers[reg].c_str());
    break;
  }
  default: {
    std::fprintf(stderr, "unrecognized pointer type.\n");
    std::exit(1);
  }
  }

  return reg;
}

int
shift_left(int reg, int value)
{
  std::fprintf(fp, "\tsalq\t$%d, %s\n", value, registers[reg].c_str());
  return reg;
}
