all:
	gcc -O2 src/main.c src/code_gen.c src/compiler.c src/lexer.c src/ast.c -o lai -g
