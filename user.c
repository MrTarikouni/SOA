#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  
  write(1,"hola",4);
  
  /*
  char buffer[11];
  for (int i = 0; i < 10; ++i) buffer[i]='/0';
  buffer[10]='/n';
	*/
  while(1) {}
}
