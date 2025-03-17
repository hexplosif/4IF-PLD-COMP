.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $65, %eax
    movl %eax, -16(%rbp)
    movl $66, %eax
    movl %eax, -12(%rbp)
    movl -16(%rbp), %eax
    movl %eax, -8(%rbp)
    movl -16(%rbp), %eax
    pushq %rax
    movl -12(%rbp), %eax
    popq %rcx
    addl %ecx, %eax
    pushq %rax
    movl -4(%rbp), %eax
    popq %rcx
    addl %ecx, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret
