.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $10, %eax
    pushq %rax
    movl $3, %eax
    popq %rcx
    subl %eax, %ecx
    movl %ecx, %eax
    pushq %rax
    movl $2, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    imull %ecx, %eax
    popq %rcx
    cmpl %eax, %ecx
    setl %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
