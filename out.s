	.text
.LC0:
	.string	"%d\n"
test_print_integer:
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
	.data
	.globl	i
i:	.long	0
	.data
	.globl	j
j:	.long	0
	.data
	.globl	ptr
ptr:	.quad	0
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$10, %r8
	movq	%r8, i(%rip)
	movq	$20, %r8
	movq	%r8, j(%rip)
	leaq	i(%rip), %r8
	movq	$1, %r9
	movq	$4, %r10
	imulq	%r9, %r10
	addq	%r8, %r10
	movq	%r10, ptr(%rip)
	movq	ptr(%rip), %r8
	movq	(%r8), %r8
	movq	%r8, %rdi
	call	test_print_integer
	movq	$0, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	popq	%rbp
	ret
