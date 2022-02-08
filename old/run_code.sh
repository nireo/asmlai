cmake --build build/
mv build/asmlai ./

rm -rf out.s
./asmlai ./test.lai
gcc -o out out.s src/libc/print_num.c
