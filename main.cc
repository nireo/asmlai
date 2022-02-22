#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "scc: invalid number of arguments\n";
    std::exit(EXIT_FAILURE);
  }

  auto tokens = token::tokenize_input(argv[1]);
  auto root_func = parser::parse_tokens(tokens);
  codegen::gen_code(root_func);

  return EXIT_SUCCESS;
}
