#include "codegen.h"
#include "compiler.h"
#include "fstream"
#include "lexer.h"
#include "parser.h"
#include <sstream>

int
main(int argc, char *argv[])
{
  if(argc != 2) {
    printf("usage: lai [input_file]\n");
    return EXIT_FAILURE;
  }

  std::string as_string = std::string(argv[1]);
  std::ifstream t(as_string);

  std::stringstream buffer;
  buffer << t.rdbuf();

  auto lexer = Lexer(buffer.str());
  auto parser = Parser(std::make_unique<Lexer>(lexer));
  auto program = parser.parse_program();

  init_out_file();
  gen_start();
  compile_ast_node(*program, -1, AstType::Program);

  end_codegen();

  return EXIT_SUCCESS;
}
