.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $15, %eax
    movl %eax, -8(%rbp)
    movl $6, %eax
    movl %eax, -4(%rbp)
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    movl %edx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
