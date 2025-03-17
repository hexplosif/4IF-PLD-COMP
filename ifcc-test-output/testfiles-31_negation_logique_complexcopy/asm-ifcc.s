.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $5, %eax
    pushq %rax
    movl $3, %eax
    pushq %rax
    movl $1, %eax
    popq %rcx
    cmpl %eax, %ecx
    setg %al
    movzbl %al, %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    andl %ecx, %eax
    popq %rcx
    xorl %ecx, %eax
    cmpl $0, %eax
    sete %al
    movzbl %al, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    cmpl %eax, %ecx
    sete %al
    movzbl %al, %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    orl %ecx, %eax
    pushq %rax
    movl $2, %eax
    popq %rcx
    cmpl %eax, %ecx
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
