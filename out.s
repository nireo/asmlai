	.text
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$-16, %rsp
	movq	$10, %r8
	movl	%r8d, -4(%rbp)
	movslq	-4(%rbp), %r8
	movq	$10, %r9
	cmpq	%r9, %r8
	jne	L2
	movslq	-4(%rbp), %r8
	movq	%r8, %rdi
	call	print_num
	movq	%rax, %r9
L2:
	movq	$0, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	addq	$16,%rsp
	popq	%rbp
	ret
