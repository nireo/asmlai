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
	.globl	fib
	.type	fib, @function
fib:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$10, %r8
	movl	%r8d, %eax
	jmp	L1
L1:
	popq %rbp
	ret
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	.comm	i,8,8
	movq	$10, %r9
	movq	%r9, %rdi
	call	fib
	movq	%rax, %r10
	movl	%r10d, i(%rip)
	movzbq	i(%rip), %r9
	movq	%r9, %rdi
	call	test_print_integer
L2:
	popq %rbp
	ret
