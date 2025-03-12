Table des symboles :
  a -> 0
  b -> 4
.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    movl $3, %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    movl %eax, -8(%rbp)
    movl -8(%rbp), %eax
    popq %rbp
    ret
