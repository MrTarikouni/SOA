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


/* Buscamos las páginas en las que mapear las páginas lógicas del hijo */
int allocate_frames(int *frames[], struct task_struct *ts_hijo) {
    for (int i = 0; i < NUM_PAG_DATA; ++i) {
        if (frames[i]=alloc_frame() < 0) {
            for (int j = 0; j < i; ++j) free_frame(frames[j]);
            list_add(&ts_hijo>list);
            return -ENUMEM;
        }//
    }
    return 1;
}


int sys_fork()
{
  int PID=-1;i

  struct task_struct* pcb_hijo;

  if (list_empty(&freequeue)) return -EFAULT;                       // Retornamos error si la freequeue está vacía
  pcb_hijo = list_head_to_task_struct(list_first(&freequeue));
  list_del(&pcb_hijo->list);

  if (allocate_DIR(pcb_hijo) == -1) {                               // Asignamos TP al hijo
      list_add(&freequeue, pcb_hijo);
      return -EFAULT;
  }
  copy_data(current(),pcb_hijo,sizeof(union task_union));           // Copiamos el PCB del padre al hijo

  union task_union *tu_hijo = (union task_union*) pcb_hijo;

  int frames[NUM_PAGE_DATA];                                        // Páginas físicas  padre de user data+stack
  if (res = allocate_frames(&frames, tu_hijo) < 0) return res;

  page_table_entry *TP_padre = get_PT(current()), *TP_hijo = get_PT(&pcb_hijo);
  for (int i = 0; i < NUM_PAGE_KERNEL; ++i)                         // Mismas direcciones del KERNEL de la TP del hijo y del padre
      set_ss_page(TP_hijo, i, get_frame(TP_padre,i));
  for (int i = PAG_LOG_INIT_CODE; i < NUM_PAG_CODE; ++i)
      set_ss_page(TP_hijo, i, get_frame(TP_padre,i));               // Las direcciones del código de usuario también se comparten


  for (int i = PAG_LOG_INIT_DATA, i < NUM_PAG_DATA; ++i)            // Entradas de la user data+stack que apuntan a las nuevas
      set_ss_page(TP_hijo, i, frames[i]));                          // páginas alocatadas



  return PID;
}

void sys_exit()
{
}

char buff[256];
//#define BUFF_SIZE 256

int sys_write(int fd, char *buffer, int size)
{

  int bytes_escritos = 0;

  int res_fd = check_fd(fd, ESCRIPTURA);
  if (res_fd < 0) return res_fd;	//Si fd da error, se retorna el código de error.
  if (buffer == NULL) return -EFAULT;	//Si el pointer del buffer es NULL se retorna error.

  int res_access = access_ok(VERIFY_WRITE,buffer,size);
  if (!res_access) return -EFAULT;

  if (size < 0) return -EFAULT;

  if (copy_from_user(buffer, buff, size)) return -EFAULT;
  bytes_escritos += sys_write_console(buff,size);

  return bytes_escritos;
}

int sys_gettime() {
	return zeos_ticks;
}
