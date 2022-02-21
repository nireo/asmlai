#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "scc: invalid number of arguments\n";
  }

  auto tokens = token::tokenize_input(argv[1]);
  auto root_node = parser::parse_tokens(tokens);
  codegen::gen_code(std::move(root_node));

  return EXIT_SUCCESS;
}
