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
	salq	$2, %r10
	addq	%r9, %r10
	leaq	arr(%rip), %r9
	movq	$0, %r11
	salq	$2, %r11
	addq	%r9, %r11
	movq	%r8, (%r11)
	movq	$11, %r8
	leaq	arr(%rip), %r9
	movq	$1, %r10
	salq	$2, %r10
	addq	%r9, %r10
	leaq	arr(%rip), %r9
	movq	$1, %r11
	salq	$2, %r11
	addq	%r9, %r11
	movq	%r8, (%r11)
	movq	$12, %r8
	leaq	arr(%rip), %r9
	movq	$2, %r10
	salq	$2, %r10
	addq	%r9, %r10
	leaq	arr(%rip), %r9
	movq	$2, %r11
	salq	$2, %r11
	addq	%r9, %r11
	movq	%r8, (%r11)
	movq	$14, %r8
	leaq	arr(%rip), %r9
	movq	$3, %r10
	salq	$2, %r10
	addq	%r9, %r10
	leaq	arr(%rip), %r9
	movq	$3, %r11
	salq	$2, %r11
	addq	%r9, %r11
	movq	%r8, (%r11)
	movq	$15, %r8
	leaq	arr(%rip), %r9
	movq	$4, %r10
	salq	$2, %r10
	addq	%r9, %r10
	leaq	arr(%rip), %r9
	movq	$4, %r11
	salq	$2, %r11
	addq	%r9, %r11
	movq	%r8, (%r11)
	leaq	arr(%rip), %r8
	movq	$1, %r9
	salq	$2, %r9
	addq	%r8, %r9
	movq	(%r9), %r9
	movq	%r9, %rdi
	call	test_print_integer
	movq	$0, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	popq	%rbp
	ret
