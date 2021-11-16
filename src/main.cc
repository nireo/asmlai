#include "codegen_x64.h"
#include "compiler.h"
#include "fstream"
#include "lexer.h"
#include "parser.h"
#include <sstream>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("usage: lai [input_file]\n");
    return EXIT_FAILURE;
  }

  std::string as_string = std::string(argv[1]);
  std::ifstream t(as_string);

  std::stringstream buffer;
  buffer << t.rdbuf();

  auto lexer = LLexer(buffer.str());
  auto parser = Parser(std::make_unique<LLexer>(lexer));
  auto program = parser.parse_program();

  codegen::init_out_file();
  codegen::gen_start();

  // init some libc functions.
  compile_ast_node(*program, -1, AstType::Program);

  codegen::end_codegen();

  return EXIT_SUCCESS;
}
