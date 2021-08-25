#include <stdio.h>
#include "code_gen.h"

int main(int argc, char **argv) {
  init_out_file();

  add_code_gen_start();
  return_zero_code_gen();
}
