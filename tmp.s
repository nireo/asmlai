  .globl main
main:
  push %rbp
  mov %rsp, %rbp
  sub $208, %rsp
  lea -8(%rbp), %rax
  push %rax
  lea -16(%rbp), %rax
  push %rax
  mov $3, %rax
  pop %rdi
  mov %rax, (%rdi)
  pop %rdi
  mov %rax, (%rdi)
  lea -16(%rbp), %rax
  mov (%rax), %rax
  push %rax
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  add %rdi, %rax
  mov %rbp, %rsp
  pop %rbp
  ret
