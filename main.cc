#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <cstdlib>
#include <iostream>

parser::Type *parser::default_int =
    new parser::Type(parser::Types::Int, parser::kNumberSize);
parser::Type *parser::default_empty = new parser::Type(parser::Types::Empty, 0);
int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "scc: invalid number of arguments\n";
    std::exit(EXIT_FAILURE);
  }

  auto tokens = token::tokenize_input(argv[1]);
  auto functions = parser::parse_tokens(tokens);
  codegen::gen_code(std::move(functions));

  delete parser::default_int;
  delete parser::default_empty;

  return EXIT_SUCCESS;
}
