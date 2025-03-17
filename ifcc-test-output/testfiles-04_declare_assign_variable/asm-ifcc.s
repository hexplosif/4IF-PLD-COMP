.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $5, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret
