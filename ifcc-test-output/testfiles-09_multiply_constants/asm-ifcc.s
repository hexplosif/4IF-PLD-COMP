.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $4, %eax
    pushq %rax
    movl $6, %eax
    popq %rcx
    imull %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
