.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax
    movl %eax, 0(%rbp)
    movl $2, %eax
    movl %eax, 4(%rbp)
    movl 0(%rbp), %eax
    pushq %rax
    movl 4(%rbp), %eax
    popq %rcx
    addl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
