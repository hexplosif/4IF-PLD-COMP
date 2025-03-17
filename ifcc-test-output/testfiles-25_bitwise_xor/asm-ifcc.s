.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $5, %eax
    movl %eax, -8(%rbp)
    movl $3, %eax
    movl %eax, -4(%rbp)
    movl $5, %eax
    pushq %rax
    movl $3, %eax
    popq %rcx
    xorl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
