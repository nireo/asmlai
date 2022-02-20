#!/bin/bash

# Inspired run tests script by github.com/rui314/9cc

assert() {
  expected="$1"
  input="$2"

  ./asmlai "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42

echo OK
