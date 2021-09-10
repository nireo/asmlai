#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>

std::unique_ptr<Program>
parse_compiler_program_helper(std::string input)
{
  auto lexer = Lexer(input);
  auto parser = Parser(std::make_unique<Lexer>(lexer));

  return parser.parse_program();
}

int
main(int argc, char *argv[])
{
  std::string input = "let x = 10";
  parse_compiler_program_helper(input);

  return EXIT_SUCCESS;
}
