# scc

A simple C compiler written purely in C++ using no dependencies. The compiler uses a recursive descent parser and compiler C code into x86-64 assembly. It uses `gcc` go compile to turn assembly code into binaries which can be then executed.

## Running compiler

```
make asmlai
./asmlai path/to/input.c -o outfile.s
```

This outputs the assembly into the `outfile.s` file.

## Running tests

```
make test
```

## Current Status

The compiler already has quite a bit of features. Namely basic types, pointers, arrays, functions and structs. The only things missing right now are macros, more types (short, long) and file includes.


