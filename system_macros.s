# 1 "system_macros.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "system_macros.S"
# 1 "include/asm.h" 1
# 2 "system_macros.S" 2

.globl writeMSR; .type writeMSR, @function; .align 0; writeMSR:
 push %ebp
 movl %esp, %ebp

 movl 0x08(%ebp), %ecx #Se mueven los parámetros
 movl 0x0C(%ebp), %edx #a los regitros
 movl 0x10(%ebp), %eax #ecx, edx, eax

 wrmsr

 movl %ebp, %esp
 popl %ebp
 ret
