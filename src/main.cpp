#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

static char *
read_file(const char *path)
{
  FILE *file = fopen(path, "rb");
  if(file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(1);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  if(buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(1);
  }

  size_t bytesRead = fread(buffer, sizeof(char), file_size, file);
  if(bytesRead < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(1);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

int
main(int argc, char **argv)
{
  if(argc != 2) {
    fprintf(stderr, "usage: lai <filepath>\n");
    exit(1);
  }

  char *source = read_file(argv[1]);
  printf("src: %s\n\n\n", source);

  auto statements = parse(source);

  // TODO: codegen for the statements.

  free(source);


  return 0;
}
