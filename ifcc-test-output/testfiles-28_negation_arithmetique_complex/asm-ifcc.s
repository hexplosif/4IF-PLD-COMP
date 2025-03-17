.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $5, %eax
    negl %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    imull %ecx, %eax
    pushq %rax
    movl $8, %eax
    popq %rcx
    addl %ecx, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    negl %eax
    movq %rbp, %rsp
    popq %rbp
    ret
