.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $7, %eax
    popq %rcx
    addl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
