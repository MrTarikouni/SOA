# 1 "wrappers.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "wrappers.S"
# 1 "include/asm.h" 1
# 2 "wrappers.S" 2


.globl write; .type write, @function; .align 0; write: #Wrapper de write
 #Guardamos el stack pointer
 pushl %ebp
 movl %esp, %ebp

 pushl %ebx

 #Pasamos los parámetros a los registros
 movl 0x08(%ebp), %ebx
 movl 0x0c(%ebp), %ecx
 movl 0x10(%ebp), %edx

 #Guardamos el id de la syscall en eax
 movl $0x04, %eax

 #Salvamos ecx y edx para que no los sobreescriba la dirección de retorno de sysexit
 pushl %ecx
 pushl %edx

 #Guardamos la dirección de retorno del user mode y fake dynamic link
 pushl $wr_return_usr

 pushl %ebp
 movl %esp, %ebp

 sysenter

wr_return_usr:

 #Popeamos de la pila los datos temporales
 leave
 addl $0x04, %esp
 popl %edx
 popl %ecx

 #Comprobamos si el valor de retorno es mayor o igual que 0
 cmpl $0, %eax
 jge wr_end

 #Si no lo es, lo devolvemos como error en valor absoluto
 negl %eax
 movl %eax, errno
 movl $-1, %eax
wr_end:
 leave
 ret


.globl gettime; .type gettime, @function; .align 0; gettime:
 #Guardamos el stack pointer
        pushl %ebp
        movl %esp, %ebp

 #Guardamos el id de la syscall en eax
        movl $0x0A, %eax

 #Salvamos ecx y edx para que no los sobreescriba la dirección de retorno de sysexit
        pushl %ecx
        pushl %edx

        #Guardamos la dirección de retorno del user mode y fake dynamic link
        pushl $gt_return_usr
        leave

 sysenter

gt_return_usr:

 popl %ebp
 addl $0x04, %ebp
 popl %edx
 popl %ecx

 #Comprobamos si el valor de retorno es mayor o igual que 0
        cmpl $0, %eax
        jge gt_end

        #Si no lo es, lo devolvemos como error en valor absoluto
        negl %eax
        movl %eax, errno
        movl $-1, %eax
gt_end:
        leave
        ret
