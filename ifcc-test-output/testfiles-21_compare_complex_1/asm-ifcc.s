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
    pushq %rax
    movl $1, %eax
    popq %rcx
    addl %ecx, %eax
    popq %rcx
    cmpl %eax, %ecx
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
