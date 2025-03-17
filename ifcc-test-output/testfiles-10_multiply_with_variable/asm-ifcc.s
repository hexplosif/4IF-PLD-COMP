.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $3, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    imull %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
