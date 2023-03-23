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

	pushl %esi
	pushl %edi
	push %ebx
	
	pushl $ts_ret
	
	push %ebp
	movl %esp, %ebp

	call inner_task_switch
ts_ret:
	leave
	addl $0x04, $esp

	popl %ebx
	popl %edi
	popl %esi
	
	leave
	ret


	

