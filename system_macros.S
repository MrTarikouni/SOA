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
	pushl %ebp
	movl %esp, %ebp
	

	pushl %esi
	pushl %edi
	pushl %ebx


	pushl 8(%ebp)	

	

	call inner_task_switch

	addl $4, %ebp

	popl %ebx
	popl %edi
	popl %esi
	

	popl %ebp
	ret

ENTRY(switch_context)
	movl 4(%esp), %eax
	movl %ebp, (%eax)
	movl 8(%esp), %esp
	
	popl %ebp
	ret




	

