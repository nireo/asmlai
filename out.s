  .file "main.c"
  .text
.LC0:
  .string	"%d\n"
print_integer:
  pushq	%rbp
  movq	%rsp, %rbp
  subq	$16, %rsp
  movl	%edi, -4(%rbp)
  movl	-4(%rbp), %eax
  movl	%eax, %esi
  leaq	.LC0(%rip), %rdi
  movl	$0, %eax
  call	printf@PLT
  nop
  leave
  ret

  .globl	main
  .type	main, @function
main:
  pushq %rbp
  movq %rsp, %rbp
  movq	$100, %r8
  movq	$100, %r9
  movq	$10, %r10
  addq	%r9, %r10
  addq	%r8, %r10
  movq	%r10, %rdi
  call	print_integer
  movl $0, %eax
  popq %rbp
  ret
