	.text
	.text
	.globl	xd
	.type	xd, @function
xd:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$0, %rsp
	movslq	16(%rbp), %r10
	movq	%r10, %rdi
	call	print_num
	movq	%rax, %r10
L1:
	addq	$0,%rsp
	popq	%rbp
	ret
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$0, %rsp
	movq	$100, %r10
	movq	%r10, %rdi
	call	xd
	movq	%rax, %r10
L2:
	addq	$0,%rsp
	popq	%rbp
	ret
