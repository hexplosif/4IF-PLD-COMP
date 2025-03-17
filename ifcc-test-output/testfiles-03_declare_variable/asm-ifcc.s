.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $0, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
