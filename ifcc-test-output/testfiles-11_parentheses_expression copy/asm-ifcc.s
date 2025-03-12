.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $2, %eax
    pushq %rax
    movl $3, %eax
    popq %rcx
    addl %ecx, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    imull %ecx, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
