#include <cstdlib>
#include <iostream>

template <typename... Args>
static void error(const char *format_string, Args... args) {
  fprintf(stderr, format_string, args...);
}

int main(int argc, char **argv) { error("hello world %d", 123); }
