/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>
#include <interrupt.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
}

char buff[256];
#define BUFF_SIZE 256

int sys_write(int fd, char *buffer, int size) 
{
 
  int bytes_escritos = 0;

  int res_fd = check_fd(fd, ESCRIPTURA);
  if (res_fd < 0) return res_fd;	//Si fd da error, se retorna el código de error.
  if (buffer == NULL) return -EFAULT;	//Si el pointer del buffer es NULL se retorna error.
  
  int res_access = access_ok(VERIFY_WRITE,buffer,size); 
  if (!res_access) return -EFAULT;

  if (size < 0) return -EFAULT;

  if (size > BUFF_SIZE) {	//Los bytes a escribir son mayores que el tamaño del buffer 
	  if (copy_from_user(buffer,buff,BUFF_SIZE)) return -EFAULT;
	  bytes_escritos += sys_write_console(buff,BUFF_SIZE);	//Se escriben los bytes que caben en el buffer
	  
	  buffer = buffer+BUFF_SIZE;
	  size = size-BUFF_SIZE;	//Se inicializa el buffer para escribir los bytes restantes
  
}

  if (copy_from_user(buffer, buff, size)) return -EFAULT;
  bytes_escritos += sys_write_console(buff,size);
  
  return bytes_escritos;
}

int sys_gettime() {
	return zeos_ticks; 
}
