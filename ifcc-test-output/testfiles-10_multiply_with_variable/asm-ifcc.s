.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    movl %eax, 0(%rbp)
    movl 0(%rbp), %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    imull %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
