Table des symboles :
  a -> 0
.globl main
 main: 
    pushq %rbp
    movq %rsp, %rbp
    movl $42, %eax
    popq %rbp
    ret
