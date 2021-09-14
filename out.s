	.file "main.c"
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

	.globl	main
	.type	main, @function
main:
	pushq %rbp
	movq %rsp, %rbp
	movq	$1, %r8
	movq	$1, %r9
	cmpq	%r9, %r8
	jne	L1
	movq	$1, %r8
	movq	$1, %r9
	addq	%r8, %r9
L1:
	movq	%r8, %rdi
	call	test_print_integer
