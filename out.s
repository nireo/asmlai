	.text
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$-16, %rsp
	movq	$100, %r10
	movl	%r10d, -4(%rbp)
	movq	$0, %r10
	movl	%r10d, %eax
	jmp	L1
L1:
	addq	$16,%rsp
	popq	%rbp
	ret
