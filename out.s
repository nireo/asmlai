	.text
	.data
	.globl	s
s:	.quad	0
	.text
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	L2(%rip), %r8
	movq	%r8, s(%rip)
L1:
	addq	$0,%rsp
	popq	%rbp
	ret
L2:
	.byte	104
	.byte	101
	.byte	108
	.byte	108
	.byte	111
	.byte	32
	.byte	119
	.byte	111
	.byte	114
	.byte	108
	.byte	100
	.byte	0
