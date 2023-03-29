#include <asm.h>

ENTRY(writeMSR)
	push %ebp
	movl %esp, %ebp

	movl 0x08(%ebp), %ecx		#Se mueven los parámetros
	movl 0x0C(%ebp), %edx		#a los regitros
	movl 0x10(%ebp), %eax		#ecx, edx, eax

	wrmsr
	
	movl %ebp, %esp
	popl %ebp
	ret


ENTRY(task_switch)
	push %ebp
	movl %esp, %ebp
	
	push 0x08(%ebp)	

	pushl %esi
	pushl %edi
	push %ebx

	call inner_task_switch

	popl %ebx
	popl %edi
	popl %esi
	
	addl $0x04, %ebp
 
	leave
	ret


ENTRY(switch_context)
	movl 0x04(%esp), %eax
	movl %ebp, (%eax)
	movl 0x08(%esp), %esp
	
	popl %ebp
	ret




	

