.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $100, %eax
    movl %eax, -8(%rbp)
    movl $25, %eax
    movl %eax, -4(%rbp)
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    pushq %rax
    movl -4(%rbp), %eax
    pushq %rax
    movl $5, %eax
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    movq %rbp, %rsp
    popq %rbp
    ret
