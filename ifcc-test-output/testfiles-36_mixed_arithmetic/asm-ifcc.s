.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $42, %eax
    movl %eax, -8(%rbp)
    movl $5, %eax
    movl %eax, -4(%rbp)
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    pushq %rax
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    movl %edx, %eax
    popq %rcx
    imull %ecx, %eax
    pushq %rax
    movl -4(%rbp), %eax
    pushq %rax
    movl -8(%rbp), %eax
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    popq %rcx
    addl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
