#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>


int
main(int argc, char *argv[])
{
  std::string input = "let x = 10;";
  parse_source(input);

  return EXIT_SUCCESS;
}
