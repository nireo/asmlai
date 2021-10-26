cmake --build build/
mv build/asmlai ./

./asmlai ./test.lai
gcc -o out out.s src/libc/print_num.c
