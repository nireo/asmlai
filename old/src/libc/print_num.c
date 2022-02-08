#include <stdio.h>

void
print_num(long n)
{
  printf("%ld\n", n);
}

void
print_char(long n)
{
  putc((char)(n & 0x7f), stdout);
}
