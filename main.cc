#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "invalid argument count " << argv[0] << '\n';
    return EXIT_FAILURE;
  }

  printf("  .globl main\n");
  printf("main:\n");
  printf("  mov $%d, %%rax\n", atoi(argv[1]));
  printf("  ret\n");
  return 0;
}
