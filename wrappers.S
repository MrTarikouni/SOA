#include <asm.h>


ENTRY(write) #Wrapper de write
	#Guardamos el stack pointer
	pushl %ebp
	movl %esp, %ebp

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

	#Fake dynamic link
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


ENTRY(gettime)
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

	#Fake dynamic link
	pushl %ebp
        movl %esp, %ebp

	sysenter

gt_return_usr:

	#Popeamos de la pila los datos temporales
        leave
        addl $0x04, %esp
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


ENTRY(fork)

        pushl %ebp
        movl %esp, %ebp

        movl $0x02, %eax

        pushl %edx
        pushl %ecx

        pushl $return_fork

        pushl %ebp
        movl %esp, %ebp

        sysenter

return_fork:

        leave

        add $0x04, %esp

        popl %ecx
        popl %edx

        cmpl $0, %eax
        jge fork_end

        negl %eax
        movl %eax, errno
        movl $-1, %eax
fork_end:
        popl %ebp
        ret

ENTRY(getpid)
        pushl %ebp
        movl %esp, %ebp

        movl $0x14, %eax

        pushl %edx
        pushl %ecx

        pushl $return_getpid

        pushl %ebp
        movl %esp, %ebp

        sysenter

return_getpid:

        leave

        add $0x04, %esp

        popl %ecx
        popl %edx

        cmpl $0, %eax
        jge getpid_end

        negl %eax
        movl %eax, errno
        movl $-1, %eax
getpid_end:
        popl %ebp
        ret


    ENTRY(exit)
    	pushl %ebp
	    mov %esp,%ebp

    	movl $1, %eax

    	pushl %edx
    	pushl %ecx

	    push $exit_ret

	    pushl %ebp
	    movl %esp,%ebp

	    # Entramos
	    sysenter

exit_ret:
	    popl %ebp
	    addl $4, %esp
	    popl %edx
	    popl %ecx
	    popl %ebp
	    ret


