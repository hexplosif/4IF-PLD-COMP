.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    pushq %rax
    movl $5, %eax
    popq %rcx
    cmpl %eax, %ecx
    setg %al
    movzbl %al, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
