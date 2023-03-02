#include<asm.h>


ENTRY(write) #Wrapper de write
	#Guardamos el stack pointer
	push %ebp
	movl %esp, %ebp
	
	#Pasamos los parámetros a los registros
	movl 0x08(%ebp), %ebx
	movl 0x0c(%ebp), %ecx
	movl 0x10(%ebp), %edx

	#Guardamos el id de la syscall en eax
	movl 0x04, %eax
 	
	#Salvamos ecx y edx para que no los sobreescriba la dirección de retorno de sysexit
	push %ecx
	push %edx
	
	#Guardamos la dirección de retorno del user mode y fake dynamic link
	push $return_usr
	push %ebp
	movl %esp, %ebp

	sysenter

return_usr:
	
	#Popeamos de la pila los datos temporales	
	popl %ebp
	popl $return_usr
	popl %edx
	popl %ecx
	
	#Comprobamos si el valor de retorno es mayor que 0
	cmpl $0, %eax
	jge end
	
	#Si no lo es, lo devolvemos como error en valor absoluto
	negl %eax
	movl %eax, errno
	move -1, %eax	
end:
	ret

		
