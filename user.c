#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */


  if (write(1,"hola\n",5) < 0) perror();

  char *buffer = "\0\0\0\0\0\0\0\0\0\0";
  write(1, "TIME: ", 6);
  itoa(gettime(), buffer);
  write(1, buffer, strlen(buffer));

  write(1, "\nPID FORK: ", 11);
  int pid = fork();
  itoa(pid, buffer);
  write(1,buffer,strlen(buffer));
/*
  write(1,"\nPID GETPID: ", 13);
  pid = getpid();
  itoa(pid,buffer);
  write(1,buffer,strlen(buffer));
*/
  /* char *p=0;
  *p = 'x'; */
  while(1) {}
}
