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
	movq	$10, %r8
	movq	$10, %r9
	addq	%r8, %r9
	movq	%r9, i(%rip)
	movq	i(%rip), %r8
	movq	$10, %r10
	addq	%r8, %r10
	movq	%r10, i(%rip)
	movl $0, %eax
	popq     %rbp
	ret
