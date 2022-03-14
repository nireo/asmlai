#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <cstdlib>
#include <iostream>

parser::Type *parser::default_int =
    new parser::Type(parser::Types::Int, parser::kNumberSize);
parser::Type *parser::default_empty = new parser::Type(parser::Types::Empty, 0);

static char *input_path;
static char *o_opt;
static void usage(int status) {
  std::fprintf(stderr, "asmlai [ -o <path> ] <file>\n");
  std::exit(status);
}

static void parse_cmd_args(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--help"))
      usage(0);

    if (!strcmp(argv[i], "-o")) {
      if (!argv[++i])
        usage(1);
      o_opt = argv[i];
      continue;
    }

    if (!strncmp(argv[i], "-o", 2)) {
      o_opt = argv[i] + 2;
      continue;
    }

    if (argv[i][0] == '-' && argv[i][1] != '\0') {
      std::fprintf(stderr, "unknown argument: %s", argv[i]);
      std::exit(1);
    }

    input_path = argv[i];
  }
}

static FILE *open_file(char *path) {
  if (!path || strcmp(path, "-") == 0)
    return stdout;

  FILE *out = std::fopen(path, "w");
  if (!out) {
    std::fprintf(stderr, "cannot open output file: %s: %s", path,
                 strerror(errno));
    std::exit(1);
  }
  return out;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "scc: invalid number of arguments\n";
    std::exit(EXIT_FAILURE);
  }

  auto tokens = token::tokenize_path(argv[1]);

  auto functions = parser::parse_tokens(tokens);
  codegen::gen_code(std::move(functions));

  delete parser::default_int;
  delete parser::default_empty;

  return EXIT_SUCCESS;
}
