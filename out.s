  .text
  .text
  .globl	print_num
  .type	print_num, @function
L1:
  popq	%rbp
  ret
  .text
  .globl	main
  .type	main, @function
main:
  pushq	%rbp
  movq	%rsp, %rbp
  movq	$125125, %r8
  movq	%r8, %rdi
  call	print_num
  movq	%rax, %r9
  movq	$0, %r8
  movl	%r8d, %eax
  jmp	L2
L2:
  popq	%rbp
  ret
