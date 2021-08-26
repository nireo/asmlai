	.file "main.c"
	.text
	.globl	main
	.type	main, @function
main:
	pushq %rbp
	movq %rsp, %rbp
	movq	$100, %r8
	movq	$512, %r9
	movq	$590, %r10
	movq	$124, %r11
	addq	%r10, %r11
	addq	%r9, %r11
	addq	%r8, %r11
	movl $0, %eax
	popq %rbp
	ret
