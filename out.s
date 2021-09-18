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
	movq	$0, %r8
	movq	%r8, i(%rip)
L1:
	movq	i(%rip), %r9
	movq	$5, %r10
	cmpq	%r10, %r9
	je	L2
	movq	i(%rip), %r8
	movq	$1, %r9
	addq	%r8, %r9
	movq	%r9, i(%rip)
	movq	i(%rip), %r8
	movq	%r8, %rdi
	call	test_print_integer
	jmp	L1
L2:
	movl $0, %eax
	popq     %rbp
	ret
