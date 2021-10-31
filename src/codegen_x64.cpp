#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "ast.h"
#include "codegen_x64.h"
#include "compiler.h"

static std::FILE *fp = nullptr;
static bool free_registers[4];
constexpr const char *registers[10] = {"%r10", "%r11", "%r12", "%r13", "%r9",
                                       "%r8",  "%rcx", "%rdx", "%rsi", "%rdi"};

constexpr const char *b_registers[10] = {"%r10b", "%r11b", "%r12b", "%r13b",
                                         "%r9b",  "%r8b",  "%cl",   "%dl",
                                         "%sil",  "%dil"};

constexpr const char *d_registers[10] = {"%r10d", "%r11d", "%r12d", "%r13d",
                                         "%r9d",  "%r8d",  "%ecx",  "%edx",
                                         "%esi",  "%edi"};

constexpr const char *jump_insts[] = {"jne", "je", "jge", "jle", "jg", "jl"};
constexpr const char *compare_instructions[] = {"sete", "setne", "setl",
                                                "setg", "setle", "setge"};

static int local_offset;
static int stack_offset;
void reset_local_offset() { local_offset = 0; }

int get_local_offset(ValueT type) {
  local_offset +=
      (get_bytesize_of_type(type) > 4) ? get_bytesize_of_type(type) : 4;
  return -local_offset;
}

static int get_corresponding_inst_index(const TokenType type) {
  switch (type) {
  case TokenType::Eq:
    return 0;
  case TokenType::Neq:
    return 1;
  case TokenType::LT:
    return 2;
  case TokenType::GT:
    return 3;
  case TokenType::ELT:
    return 4;
  case TokenType::EGT:
    return 5;
  default:
    std::fprintf(stderr, "no free registers\n");
    std::exit(1);
  }
}

int get_bytesize_of_type(ValueT type) {
  switch (type) {
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

enum { no_seg, text_seg, data_seg } curr_seg = no_seg;

void textseg() {
  if (curr_seg != data_seg) {
    std::fputs("\t.text\n", fp);
    curr_seg = text_seg;
  }
}

void dataseg() {
  if (curr_seg != data_seg) {
    std::fputs("\t.data\n", fp);
    curr_seg = data_seg;
  }
}

void free_all_registers() {
  free_registers[0] = true;
  free_registers[1] = true;
  free_registers[2] = true;
  free_registers[3] = true;
}

static int get_register() {
  for (int i = 0; i < 4; ++i) {
    if (free_registers[i]) {
      free_registers[i] = false;
      return i;
    }
  }

  std::fprintf(stderr, "no free registers\n");
  std::exit(1);
}

static void free_register(int reg) {
  if (free_registers[reg] != 0) {
    std::fprintf(stderr, "register is already free");
    std::exit(1);
  }

  free_registers[reg] = true;
}

void init_out_file() {
  if ((fp = fopen("out.s", "w")) == NULL) {
    std::fprintf(stderr, "unable to create out file\n");
    std::exit(1);
  }
}

void gen_start() {
  free_all_registers();
  std::fprintf(fp, "\t.text\n");
}

int load_into_register(int val) {
  int reg = get_register();
  fprintf(fp, "\tmovq\t$%d, %s\n", val, registers[reg]);

  return reg;
}

int mul_registers(int r1, int r2) {
  fprintf(fp, "\timulq\t%s, %s\n", registers[r1], registers[r2]);
  free_register(r1);

  return r2;
}

int div_registers(int r1, int r2) {
  fprintf(fp, "\tmovq\t%s,%%rax\n\tcqo\n", registers[r1]);
  fprintf(fp, "\tidivq\t%s\n", registers[r2]);
  fprintf(fp, "\tmovq\t%%rax,%s\n", registers[r1]);
  free_register(r2);

  return r1;
}

int add_registers(int reg1, int reg2) {
  fprintf(fp, "\taddq\t%s, %s\n", registers[reg1], registers[reg2]);
  free_register(reg1);

  return reg2;
}

int sub_registers(int reg1, int reg2) {
  fprintf(fp, "\tsubq\t%s, %s\n", registers[reg2], registers[reg1]);
  free_register(reg2);

  return reg1;
}

void print_register(int r) {
  fprintf(fp,
          "\tmovq\t%s, %%rdi\n"
          "\tcall\ttest_print_integer\n",
          registers[r]);
  free_register(r);
}

void end_codegen() { fclose(fp); }

int store_global(int r, const Symbol &sym) {
  switch (sym.value_type_) {
  case TYPE_CHAR: {
    std::fprintf(fp, "\tmovb\t%s, %s(\%%rip)\n", b_registers[r],
                 sym.name_.c_str());
    break;
  }
  case TYPE_INT: {
    std::fprintf(fp, "\tmovl\t%s, %s(\%%rip)\n", d_registers[r],
                 sym.name_.c_str());
    break;
  }
  case TYPE_LONG: {
    std::fprintf(fp, "\tmovq\t%s, %s(\%%rip)\n", registers[r],
                 sym.name_.c_str());
    break;
  }
  case TYPE_PTR_CHAR:
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    std::fprintf(fp, "\tmovq\t%s, %s(\%%rip)\n", registers[r],
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

void generate_sym(const Symbol &sym) {
  int size = get_bytesize_of_type(sym.value_type_);
  std::fprintf(fp,
               "\t.data\n"
               "\t.globl\t%s\n%s:",
               sym.name_.c_str(), sym.name_.c_str());

  for (int i = 0; i < sym.size; ++i) {
    switch (size) {
    case 1:
      std::fprintf(fp, "\t.byte\t0\n");
      break;
    case 4:
      std::fprintf(fp, "\t.long\t0\n");
      break;
    case 8:
      std::fprintf(fp, "\t.quad\t0\n");
      break;
    default:
      std::fprintf(stderr, "unrecognized byte size.");
      std::exit(1);
    }
  }
}

int load_global(const Symbol &sym, TokenType opr, bool post) {
  int free_reg = get_register();

  switch (sym.value_type_) {
  case TYPE_CHAR: {
    if (!post) {
      if (opr == TokenType::Inc) {
        std::fprintf(fp, "\tincb\t%s(\%%rip)\n", sym.name_.c_str());
      }

      if (opr == TokenType::Dec) {
        std::fprintf(fp, "\tdecb\t%s(\%%rip)\n", sym.name_.c_str());
      }
    }

    std::fprintf(fp, "\tmovzbq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc) {
        std::fprintf(fp, "\tincb\t%s(\%%rip)\n", sym.name_.c_str());
      }

      if (opr == TokenType::Dec) {
        std::fprintf(fp, "\tdecb\t%s(\%%rip)\n", sym.name_.c_str());
      }
    }

    break;
  }
  case TYPE_INT: {
    if (!post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincl\t%s(\%%rip)\n", sym.name_.c_str());
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecl\t%s(\%%rip)\n", sym.name_.c_str());
    }

    std::fprintf(fp, "\tmovslq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincl\t%s(\%%rip)\n", sym.name_.c_str());
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecl\t%s(\%%rip)\n", sym.name_.c_str());
    }

    break;
  }
  case TYPE_LONG:
  case TYPE_PTR_CHAR:
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    if (!post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincq\t%s(\%%rip)\n", sym.name_.c_str());
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecq\t%s(\%%rip)\n", sym.name_.c_str());
    }

    std::fprintf(fp, "\tmovq\t%s(\%%rip), %s\n", sym.name_.c_str(),
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincq\t%s(\%%rip)\n", sym.name_.c_str());
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecq\t%s(\%%rip)\n", sym.name_.c_str());
    }

    break;
  }
  default: {
    std::fprintf(stderr, "cannot load global type.\n");
    std::exit(1);
  }
  }

  return free_reg;
}

int codegen_compare_no_jump(int reg1, int reg2, const TokenType type) {
  auto index = get_corresponding_inst_index(type);

  std::fprintf(fp, "\tcmpq\t%s, %s\n", registers[reg2], registers[reg1]);
  std::fprintf(fp, "\t%s\t%s\n", compare_instructions[index],
               b_registers[reg2]);
  std::fprintf(fp, "\tmovzbq\t%s, %s\n", b_registers[reg2], registers[reg2]);

  free_register(reg1);

  return reg2;
}

int codegen_compare_jump(int reg1, int reg2, int label, const TokenType type) {
  auto index = get_corresponding_inst_index(type);

  std::fprintf(fp, "\tcmpq\t%s, %s\n", registers[reg2], registers[reg1]);
  std::fprintf(fp, "\t%s\tL%d\n", jump_insts[index], label);
  free_all_registers();

  return -1;
}

void gen_label(int label) {
  std::cout << "label: " << label << '\n';
  if (fp == nullptr) {
    if ((fp = fopen("out.s", "a")) == NULL) {
      std::fprintf(stderr, "unable to reopen out file\n");
      std::exit(1);
    }
  }

  std::fprintf(fp, "L%d:\n", label);
}

void gen_jmp(int label) { std::fprintf(fp, "\tjmp\tL%d\n", label); }

void function_start(const std::string &name) {
  textseg();
  local_offset = 0;

  auto nc = name.c_str();
  std::fprintf(fp,
               "\t.globl\t%s\n"
               "\t.type\t%s, @function\n"
               "%s:\n"
               "\tpushq\t%%rbp\n"
               "\tmovq\t%%rsp, %%rbp\n",
               nc, nc, nc);

  int param_register = 9;
  auto &locals = get_symbol_table(Scope::Local);
  for (auto &[s, sym] : locals) {
    if (sym.st_type != Scope::Parameter)
      continue;

    std::cout << s << '\n';

    sym.position = get_local_offset(sym.value_type_);
    std::cout << sym.position;
    store_local(sym, param_register--);
  }

  int param_offset = 16;
  for (auto &[s, sym] : locals) {
    if (sym.st_type == Scope::Parameter) {
      sym.position = param_offset;
      param_offset += 8;
    } else {
      sym.position = get_local_offset(sym.value_type_);
    }
  }

  stack_offset = (local_offset + 15) & ~15;
  std::fprintf(fp, "\taddq\t$%d, %%rsp\n", -stack_offset);
}

void codegen_return(int reg, const Symbol &sym) {
  switch (sym.value_type_) {
  case TYPE_CHAR:
    fprintf(fp, "\tmovzbl\t%s, %%eax\n", b_registers[reg]);
    break;
  case TYPE_INT:
    fprintf(fp, "\tmovl\t%s, %%eax\n", d_registers[reg]);
    break;
  case TYPE_LONG:
    std::fprintf(fp, "\tmovq\t%s, %%rax\n", registers[reg]);
    break;
  default: {
    std::fprintf(stderr, "undefined return function");
    std::exit(1);
  }
  }

  gen_jmp(sym.label);
}

int codegen_call(const std::string &name, int argument_count) {
  int outer = get_register();
  // std::fprintf(fp, "\tmovq\t%s, %%rdi\n", registers[reg]);
  std::fprintf(fp, "\tcall\t%s\n", name.c_str());
  if (argument_count > 6)
    std::fprintf(fp, "\taddq\t$%d, %%rsp\n", 8 * (argument_count - 6));
  std::fprintf(fp, "\tmovq\t%%rax, %s\n", registers[outer]);

  return outer;
}

void function_end(int label) {
  gen_label(label);
  std::fprintf(fp, "\taddq\t$%d,%%rsp\n", stack_offset);

  std::fputs("\tpopq	%rbp\n"
             "\tret\n",
             fp);
}

int codegen_addr(const Symbol &sym) {
  int r = get_register();

  std::fprintf(fp, "\tleaq\t%s(%%rip), %s\n", sym.name_.c_str(), registers[r]);

  return r;
}

int codegen_dereference(int reg, const ValueT type) {
  switch (type) {
  case TYPE_PTR_CHAR: {
    std::fprintf(fp, "\tmovzbq\t(%s), %s\n", registers[reg], registers[reg]);
    break;
  }
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    std::fprintf(fp, "\tmovq\t(%s), %s\n", registers[reg], registers[reg]);
    break;
  }
  default: {
    std::fprintf(stderr, "unrecognized pointer type.\n");
    std::exit(1);
  }
  }

  return reg;
}

int shift_left(int reg, int value) {
  std::fprintf(fp, "\tsalq\t$%d, %s\n", value, registers[reg]);
  return reg;
}

int shift_left_from_reg(int r1, int r2) {
  std::fprintf(fp, "\tmovb\t%s, %%cl\n", b_registers[r2]);
  std::fprintf(fp, "\tshlq\t%%cl, %s\n", registers[r1]);

  free_register(r2);
  return r1;
}

int shift_right_from_reg(int r1, int r2) {
  std::fprintf(fp, "\tmovb\t%s, %%cl\n", b_registers[r2]);
  std::fprintf(fp, "\tshrq\t%%cl, %s\n", registers[r1]);
  free_register(r2);

  return r1;
}

int codegen_and(int r1, int r2) {
  std::fprintf(fp, "\tandq\t%s, %s\n", registers[r1], registers[r2]);

  free_register(r1);
  return r2;
}

int codegen_or(int r1, int r2) {
  std::fprintf(fp, "\torq\t%s, %s\n", registers[r1], registers[r2]);

  free_register(r1);
  return r2;
}

int codegen_load_int(int value) {
  int free_reg = get_register();

  std::fprintf(fp, "\tmovq\t$%d, %s\n", value, registers[free_reg]);
  return free_reg;
}

int convert_into_bool(int r, AstType node_type, int label) {
  if (node_type == AstType::IfExpression ||
      node_type == AstType::WhileStatement) {
    std::fprintf(fp, "\tje\tL%d\n", label);
  } else {
    std::fprintf(fp, "\tsetnz\t%s\n", b_registers[r]);
    std::fprintf(fp, "\tmovzbq\t%s, %s\n", b_registers[r], registers[r]);
  }

  return r;
}

int codegen_invert(int r) {
  std::fprintf(fp, "\tnotq\t%s\n", registers[r]);
  return r;
}

int codegen_neg(int r) {
  std::fprintf(fp, "\tnegq\t%s\n", registers[r]);
  return r;
}

int codegen_not(int r) {
  std::fprintf(fp, "\ttest\t%s, %s\n", registers[r], registers[r]);
  std::fprintf(fp, "\tsete\t%s\n", b_registers[r]);
  std::fprintf(fp, "\tmovzbq\t%s, %s\n", b_registers[r], registers[r]);

  return r;
}

int store_dereference(int reg1, int reg2, ValueT type) {
  switch (type) {
  case TYPE_CHAR:
    std::fprintf(fp, "\tmovb\t%s, (%s)\n", b_registers[reg1], registers[reg2]);
    break;
  case TYPE_INT:
    std::fprintf(fp, "\tmovq\t%s, (%s)\n", registers[reg1], registers[reg2]);
    break;
  case TYPE_LONG:
    std::fprintf(fp, "\tmovq\t%s, (%s)\n", registers[reg1], registers[reg2]);
    break;
  default:
    std::fprintf(stderr, "uncompatible type for deference storing.");
    std::exit(1);
  }

  return reg1;
}

void global_str(int l, const std::string &value) {
  gen_label(l);
  for (size_t i = 0; i < value.length(); i++) {
    std::fprintf(fp, "\t.byte\t%d\n", value[i]);
  }

  std::fprintf(fp, "\t.byte\t0\n");
}

int codegen_xor(int r1, int r2) {
  std::fprintf(fp, "\txorq\t%s, %s\n", registers[r1], registers[r2]);
  free_register(r1);

  return r2;
}

int load_global_str(int l) {
  int r = get_register();
  std::fprintf(fp, "\tleaq\tL%d(\%%rip), %s\n", l, registers[r]);

  return r;
}

int load_local(const Symbol &sym, TokenType opr, bool post) {
  int free_reg = get_register();

  switch (sym.value_type_) {
  case TYPE_CHAR: {
    if (!post) {
      if (opr == TokenType::Inc) {
        std::fprintf(fp, "\tincb\t%d(\%%rip)\n", sym.position);
      }

      if (opr == TokenType::Dec) {
        std::fprintf(fp, "\tdecb\t%d(\%%rip)\n", sym.position);
      }
    }

    std::fprintf(fp, "\tmovzbq\t%d(\%%rip), %s\n", sym.position,
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc) {
        std::fprintf(fp, "\tincb\t%d(\%%rip)\n", sym.position);
      }

      if (opr == TokenType::Dec) {
        std::fprintf(fp, "\tdecb\t%d(\%%rip)\n", sym.position);
      }
    }

    break;
  }
  case TYPE_INT: {
    if (!post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincl\t%d(\%%rip)\n", sym.position);
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecl\t%d(\%%rip)\n", sym.position);
    }

    std::fprintf(fp, "\tmovslq\t%d(\%%rbp), %s\n", sym.position,
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincl\t%d(\%%rip)\n", sym.position);
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecl\t%d(\%%rip)\n", sym.position);
    }

    break;
  }
  case TYPE_LONG:
  case TYPE_PTR_CHAR:
  case TYPE_PTR_LONG:
  case TYPE_PTR_INT: {
    if (!post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincq\t%d(\%%rip)\n", sym.position);
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecq\t%d(\%%rip)\n", sym.position);
    }

    std::fprintf(fp, "\tmovq\t%d(\%%rip), %s\n", sym.position,
                 registers[free_reg]);

    if (post) {
      if (opr == TokenType::Inc)
        fprintf(fp, "\tincq\t%d(\%%rip)\n", sym.position);
      if (opr == TokenType::Dec)
        fprintf(fp, "\tdecq\t%d(\%%rip)\n", sym.position);
    }

    break;
  }
  default: {
    std::fprintf(stderr, "cannot load local.\n");
    std::exit(1);
  }
  }

  return free_reg;
}

void copy_argument(int r, int pos) {
  if (pos > 6) {
    std::fprintf(fp, "\tpushq\t%s\n", registers[r]);
  } else {
    std::fprintf(fp, "\tmovq\t%s, %s\n", registers[r], registers[9 - pos + 1]);
  }
}

int store_local(const Symbol &sym, int r) {
  std::cout << ' ' << sym.position << '\n';
  switch (sym.value_type_) {
  case TYPE_CHAR:
    std::fprintf(fp, "\tmovb\t%s, %d(%%rbp)\n", b_registers[r], sym.position);
    break;
  case TYPE_INT:
    std::fprintf(fp, "\tmovl\t%s, %d(%%rbp)\n", d_registers[r], sym.position);
    break;
  case TYPE_LONG:
  case TYPE_PTR_CHAR:
  case TYPE_PTR_INT:
  case TYPE_PTR_LONG:
    std::fprintf(fp, "\tmovq\t%s, %d(%%rbp)\n", registers[r], sym.position);
    break;
  default: {
    std::fprintf(stderr, "cannot store local");
    std::exit(1);
  }
  }

  return r;
}
