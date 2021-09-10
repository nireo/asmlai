#include "lexer.h"
#include "parser.h"

int
main(int argc, char *argv[])
{
  std::string input = "let x = 10;";
  auto lexer = Lexer(input);
  auto parser = Parser(std::make_unique<Lexer>(lexer));

  auto program = parser.parse_program();

  return EXIT_SUCCESS;
}
