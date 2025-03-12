.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $2, %eax
    pushq %rax
    movl $3, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    imull %ecx, %eax
    popq %rcx
    addl %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
