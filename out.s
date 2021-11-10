	.text
	.data
	.globl	buf
buf:	.quad	0
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	addq	$-16, %rsp
	leaq	L6(%rip), %r10
	movq	%r10, buf(%rip)
	movq	$0, %r10
	movq	%r10, %rdi
	leaq	L7(%rip), %r10
	movq	%r10, %rsi
	call	open@PLT
	movq	%rax, %r10
	movl	%r10d, -12(%rbp)
	movslq	-12(%rbp), %r10
	movslq	-4(%rbp), %r11
	cmpq	%r11, %r10
	jne	L8
	movq	$100, %r10
	movq	%r10, %rdi
	call	print_num@PLT
	movq	%rax, %r10
	movq	$1, %r10
	movl	%r10d, %eax
	jmp	L5
L8:
	movq	$60, %r10
	movq	%r10, %rdi
	movq	buf(%rip), %r10
	movq	%r10, %rsi
	movslq	-12(%rbp), %r10
	movq	%r10, %rdx
	call	read@PLT
	movq	%rax, %r10
	movl	%r10d, -8(%rbp)
L9:
	movslq	-8(%rbp), %r10
	movq	$0, %r11
	movb	%r11b, %cl
	shrq	%cl, %r10
	movslq	-8(%rbp), %r10
	movq	%r10, %rdi
	movq	buf(%rip), %r10
	movq	%r10, %rsi
	movq	$1, %r10
	movq	%r10, %rdx
	call	write@PLT
	movq	%rax, %r10
	movq	$60, %r10
	movq	%r10, %rdi
	movq	buf(%rip), %r10
	movq	%r10, %rsi
	movslq	-12(%rbp), %r10
	movq	%r10, %rdx
	call	read@PLT
	movq	%rax, %r10
	movl	%r10d, -8(%rbp)
	jmp	L9
L10:
	movslq	-12(%rbp), %r10
	movq	%r10, %rdi
	call	close@PLT
	movq	%rax, %r10
	movq	$0, %r10
	movl	%r10d, %eax
	jmp	L5
L5:
	addq	$16,%rsp
	popq	%rbp
	ret
L6:
	.byte	34
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	32
	.byte	34
	.byte	0
L7:
	.byte	34
	.byte	126
	.byte	47
	.byte	100
	.byte	101
	.byte	118
	.byte	47
	.byte	97
	.byte	115
	.byte	109
	.byte	108
	.byte	97
	.byte	105
	.byte	47
	.byte	116
	.byte	101
	.byte	115
	.byte	116
	.byte	46
	.byte	108
	.byte	97
	.byte	105
	.byte	34
	.byte	0
