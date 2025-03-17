.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $5, %eax
    pushq %rax
    movl $8, %eax
    popq %rcx
    cmpl %eax, %ecx
    setl %al
    movzbl %al, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    cmpl $0, %eax
    sete %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
