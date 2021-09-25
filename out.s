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
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	.comm	i,8,8
	movq	$10, %r8
	movl	%r8d, i(%rip)
	.comm	ptr,8,8
	leaq	i(%rip), %r9
	movq	%r9, ptr(%rip)
	.comm	after_deref,8,8
	movq	ptr(%rip), %r10
	movq	(%r10), %r10
	movl	%r10d, after_deref(%rip)
	movzbq	after_deref(%rip), %r11
	movq	%r11, %rdi
	call	test_print_integer
	movq	$0, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	popq %rbp
	ret
