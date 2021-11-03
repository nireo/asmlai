	.text
	.text
	.globl	fib
	.type	fib, @function
fib:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%edi, -4(%rbp)
	addq	$-16, %rsp
	movslq	-4(%rbp), %r10
	movq	$0, %r11
	cmpq	%r11, %r10
	jne	L3
	movq	$0, %r10
	movl	%r10d, %eax
	jmp	L1
L3:
	movslq	-4(%rbp), %r10
	movq	$1, %r11
	cmpq	%r11, %r10
	jne	L4
	movq	$1, %r10
	movl	%r10d, %eax
	jmp	L1
L4:
	movslq	-4(%rbp), %r10
	movq	$1, %r11
	subq	%r11, %r10
	movq	%r10, %rdi
	call	fib
	movq	%rax, %r10
	movslq	-4(%rbp), %r11
	movq	$2, %r12
	subq	%r12, %r11
	movq	%r11, %rdi
	call	fib
	movq	%rax, %r10
	addq	%r10, %r10
	movl	%r10d, %eax
	jmp	L1
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
	movq	$36, %r10
	movq	%r10, %rdi
	call	fib
	movq	%rax, %r10
	movq	%r10, %rdi
	call	print_num
	movq	%rax, %r10
	movq	$0, %r10
	movl	%r10d, %eax
	jmp	L2
L2:
	addq	$0,%rsp
	popq	%rbp
	ret
