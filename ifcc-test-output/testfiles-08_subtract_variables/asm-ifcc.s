.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $8, %eax
    movl %eax, -8(%rbp)
    movl $5, %eax
    movl %eax, -4(%rbp)
    movl -8(%rbp), %eax
    pushq %rax
    movl -4(%rbp), %eax
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
