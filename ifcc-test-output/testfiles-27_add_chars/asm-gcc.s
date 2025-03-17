	.file	"input.c"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movb	$65, -1(%rbp)
	movb	$66, -2(%rbp)
	movsbl	-1(%rbp), %eax
	movl	%eax, -8(%rbp)
	movsbl	-1(%rbp), %edx
	movsbl	-2(%rbp), %eax
	addl	%edx, %eax
	addl	%eax, -12(%rbp)
	movl	-12(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Debian 12.2.0-14) 12.2.0"
	.section	.note.GNU-stack,"",@progbits
