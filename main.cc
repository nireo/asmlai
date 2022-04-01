#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <cstdlib>
#include <iostream>

parser::Type *parser::default_int = new parser::Type(
    parser::Types::Int, parser::kNumberSize, parser::kNumberSize);
parser::Type *parser::default_empty = new parser::Type(parser::Types::Empty, 0);
parser::Type *parser::default_void =
    new parser::Type(parser::Types::Void, 1, 1);
parser::Type *parser::default_long =
    new parser::Type(parser::Types::Long, parser::kLongSize, parser::kLongSize);

parser::Scope *parser::scopes = new parser::Scope();

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

  if (!input_path) {
    std::fprintf(stderr, "no input files.");
    std::exit(1);
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
  parse_cmd_args(argc, argv);

  auto tokens = token::tokenize_path(input_path);

  auto functions = parser::parse_tokens(tokens);
  FILE *out = open_file(o_opt);
  fprintf(out, ".file 1 \"%s\"\n", input_path);
  codegen::gen_code(std::move(functions), out);

  delete parser::default_int;
  delete parser::default_empty;

  return EXIT_SUCCESS;
}
