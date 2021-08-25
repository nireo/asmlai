	.file "main.c"
	.text
	.globl	main
	.type	main, @function
main:
	pushq %rbp
	movq %rsp, %rbp
	movl $0, %eax
	popq %rbp
	ret
