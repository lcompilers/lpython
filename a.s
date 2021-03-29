	.text
	.file	"LFortran"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %.entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movabsq	$4294967296, %rax               # imm = 0x100000000
	movq	%rax, -32(%rbp)
	movabsq	$21474836481, %rax              # imm = 0x500000001
	movq	%rax, -24(%rbp)
	movl	$5, -16(%rbp)
	movq	%rsp, %rax
	leaq	-48(%rax), %rsp
	movq	-48(%rax), %rax
	movq	%rax, -40(%rbp)
	movl	$20, -4(%rbp)
	movl	$2, %ecx
	subl	-24(%rbp), %ecx
	movslq	%ecx, %rcx
	shlq	$6, %rcx
	movl	$3, (%rax,%rcx)
	xorl	%eax, %eax
	movq	%rbp, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.L__unnamed_1,@object           # @0
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__unnamed_1:
	.asciz	"%d\n"
	.size	.L__unnamed_1, 4

	.section	".note.GNU-stack","",@progbits
