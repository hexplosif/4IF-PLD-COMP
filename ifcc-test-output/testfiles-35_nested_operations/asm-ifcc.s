.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $1, %eax
    movl %eax, -12(%rbp)
    movl $2, %eax
    movl %eax, -8(%rbp)
    movl $3, %eax
    movl %eax, -4(%rbp)
    movl -12(%rbp), %eax
    pushq %rax
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    popq %rcx
    imull %ecx, %eax
    popq %rcx
    addl %ecx, %eax
    pushq %rax
    movl -12(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    pushq %rax
    movl -8(%rbp), %eax
    pushq %rax
    popq %rcx
    popq %rax
    cltd
    idivl %ecx
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
