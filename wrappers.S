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

	pushl $fk_return_usr

	pushl %ebp
	movl %esp, %ebp

	sysenter

fk_return_usr:
		
		leave
		add $0x04, %esp
		popl %ecx
		popl %edx

		cmpl $0, %eax
		jge fk_end

		negl %eax
		movl %eax, errno
		movl $-1, %eax
fk_end:
		
		popl %ebp
		ret

ENTRY(getpid)
	pushl %ebp
	movl %esp, %ebp

	movl $0x14, %eax

	pushl %edx
	pushl %ecx

	pushl $gp_return_usr

	pushl %ebp
	movl %esp, %ebp

	sysenter

gp_return_usr:
	leave
	add $0x04, %esp
	popl %ecx
	popl %edx
	cmpl $0, %eax

	jge gp_end

	negl %eax
	movl %eax, errno
	movl $-1, %eax
gp_end:
	popl %ebp
	ret

ENTRY(exit) 
	pushl %ebp
	movl %esp, %ebp

	movl $0x1, %eax

	pushl %edx
	pushl %ecx

	pushl $exit_return_usr

	pushl %ebp
	movl %esp, %ebp

	sysenter

exit_return_usr:

	popl %ebp
	addl $4, %esp
	popl %edx
	popl %ecx
	popl %ebp    
	ret

