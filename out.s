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
	.globl	arr
arr:	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$10, %r8
	leaq	arr(%rip), %r9
	movq	$0, %r10
	movq	$4, %r11
	imulq	%r10, %r11
	addq	%r9, %r11
	leaq	arr(%rip), %r9
	movq	$0, %r10
