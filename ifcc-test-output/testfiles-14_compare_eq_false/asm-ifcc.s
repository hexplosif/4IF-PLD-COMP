.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    cmpl %eax, %ecx
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
