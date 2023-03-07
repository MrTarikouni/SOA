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

