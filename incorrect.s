  .globl main
main:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  lea -8(%rbp), %rax
  push %rax
  mov $3, %rax
  pop %rdi
  mov %rax, (%rdi)
  lea -16(%rbp), %rax
  push %rax
  mov $5, %rax
  pop %rdi
  mov %rax, (%rdi)
  mov $8, %rax
  push %rax
  lea -16(%rbp), %rax
  pop %rdi
  sub %rdi, %rax
  push %rax
  mov $7, %rax
  pop %rdi
  mov %rax, (%rdi)
  lea -8(%rbp), %rax
  mov (%rax), %rax
  jmp .L.return
.L.return:
  mov %rbp, %rsp
  pop %rbp
  ret
