	.text
	.data
	.globl	x
x:	.long	0
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$69, %r8
	movq	%r8, x(%rip)
	movzbq	x(%rip), %r8
	incl	x(%rip)
	movzbq	x(%rip), %r8
	movq	%r8, %rdi
	call	print_num
	movq	%rax, %r9
L1:
	popq	%rbp
	ret
