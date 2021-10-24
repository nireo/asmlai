	.text
	.data
	.globl	i
i:	.long	0
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$-16,%rsp
	movq	$10, %r8
	movl	%r8d, -4(%rbp)
	movq	$0, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	addq	$16,%rsp
	popq	%rbp
	ret
