	.text
	.text
	.globl	xd
	.type	xd, @function
xd:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	addq	$-16, %rsp
	movslq	-8(%rbp), %r10
	movq	%r10, %rdi
	call	print_num
	movq	%rax, %r10
	movslq	-4(%rbp), %r10
	movq	%r10, %rdi
	call	print_num
	movq	%rax, %r10
L1:
	addq	$16,%rsp
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
	movq	$10, %r10
	movq	%r10, %rsi
	call	xd
	movq	%rax, %r10
	movq	$0, %r10
	movl	%r10d, %eax
	jmp	L2
L2:
	addq	$0,%rsp
	popq	%rbp
	ret
