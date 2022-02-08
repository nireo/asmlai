# asmlai

This project contains a parser for the `lai` language. Currently it only compiles into x64 assembly, and can be turned into an executable through the `gcc` assembler.

## Examples
```
// pointers
fn swap(a: int*, b: int*) => void {
   var int temp;
   temp = *a;
   *a = *b;
   *b = temp;
}

// recursive functions and control flow
fn fib(n: int) => int {
    if (n <= 1) {
        return n;
    }

    return fib(n-1) + fib(n-2);
}

// other
fn main() => void {
    let int i;
    i = 0;
    while (i != 5) {
        print i;
        i++;
    }
}
```

## Todos
* Function calls
* Safer type system
* Pointers
* Optimizations
* Arrays
* Structs
* Unions
* Preprocessor
