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

int ret_from_fork() {
    return 0;
}



/* Copia la user data+stak del padre al hijo */
void copy_pag_data(page_table_entry *TP_padre, page_table_entry *TP_hijo) {
    for (int i = NUM_PAG_KERNEL+NUM_PAG_CODE; i < NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; ++i) {
        /* mapeamos la página física del hijo en la página lógica del padre de la región vacía de la TP */
        set_ss_pag(TP_padre, i+NUM_PAG_DATA, get_frame(TP_hijo, i));
        /* copiamos los datos con los mapeos que tenemos en la TP del padre */
        copy_data((void *)((i) << 12), (void *)((i+NUM_PAG_DATA) << 12), PAGE_SIZE);
        /* liberamos la página lógica de la TP del padre que mapeaba la página física del hijo */
        del_ss_pag(TP_padre, i+NUM_PAG_DATA);
    }

}

unsigned int pid_count = 1;

int sys_fork()
{
  // Retornamos error si la freequeue está vacía
  if (list_empty(&freequeue)) return -ENOMEM;
  struct task_struct* pcb_hijo = list_head_to_task_struct(list_first(&freequeue));
  list_del(pcb_hijo->list);

 // Copiamos el PCB del padre al hijo
  copy_data(current(),(union task_union *)pcb_hijo,sizeof(union task_union));

  // Asignamos TP al hijo
  if (allocate_DIR(pcb_hijo) == -1) {
      list_add(pcb_hijo->list, &freequeue);
      return -ENOMEM;
  }

  union task_union *tu_hijo = (union task_union*) pcb_hijo;

  page_table_entry *TP_padre = get_PT(current()), *TP_hijo = get_PT(pcb_hijo);

  // Páginas físicas  padre de user data+stack
  int frames[NUM_PAG_DATA];
  /* Buscamos las páginas en las que mapear las páginas lógicas del hijo */
  for (int i = 0; i < NUM_PAG_DATA; ++i) {
      frames[i]=alloc_frame();
      if (frames[i]< 0) {
          for (int j = 0; j < i; ++j) free_frame(frames[j]);
          list_add_tail(pcb_hijo->list, &freequeue);
          return -ENOMEM;
        }//
    }


  // Mismas direcciones del KERNEL de la TP del hijo y del padre
  for (int i = 0; i < NUM_PAG_KERNEL; ++i)
      set_ss_pag(TP_hijo, i, get_frame(TP_padre,i));
  // Las direcciones del código de usuario también se comparten
  for (int i = PAG_LOG_INIT_CODE; i < PAG_LOG_INIT_CODE+NUM_PAG_CODE; ++i)
      set_ss_pag(TP_hijo, i, get_frame(TP_padre,i));


  // Entradas de la user data+stack que apuntan a las NUM_PAG_DATA
  for (int i = 0; i < NUM_PAG_DATA; ++i)
      set_ss_pag(TP_hijo, PAG_LOG_INIT_DATA+i, frames[i]);

  copy_pag_data(TP_padre, TP_hijo);

  // Flush TLB para que el padre no las traducciones del hijo
  set_cr3(get_DIR(current()));
  // Asignamos un PID=PID del anterior proceso creado +1
  pcb_hijo->PID=++pid_count;
  pcb_hijo->st=ST_READY;

/*

   PILA DE SISTEMA HIJO:

    POS
           -------------
   1005 -> |     0     | <--- "EBP"
           -------------
   1006 -> |@ret_f_fork|
           -------------
   1007    |  @handler |
           -------------\
   1008 -> |    EBX    | \
           -------------  \
           |    ....   |   \
           -------------   /--> Ctxt sw
           |    FS     |  /
           ------------- /
   1018 -> |    GS     |/
           -------------\
           |    EIP    | \
           -------------  \
           |    CS     |   \
           -------------    \
           |    PSW    |     \--> Ctxt hw
           -------------    /
   1022 -> |    ESP    |   /
           -------------  /
   1023 -> |    SS     | /
           -------------/
*/

  tu_hijo->stack[1005]=(unsigned long) 0;
  tu_hijo->stack[1006]=(unsigned long) &ret_from_fork;
  pcb_hijo->kernel_esp=(unsigned long)&tu_hijo->stack[1005];

  // añadimos al hijo en la readyqueue
  list_add_tail(&pcb_hijo->list, &readyqueue);

  return pcb_hijo->PID;
}

void sys_exit()
{
    struct task_struct *actual = current();
    page_table_entry *TP_actual = get_PT(actual);

    for (int i = 0; i < NUM_PAG_DATA; ++i){
      unsigned int frame = get_frame(TP_actual,PAG_LOG_INIT_DATA+i);
      free_frame(frame);
      del_ss_pag(TP_actual,PAG_LOG_INIT_DATA+i);
    }
    //free_user_pages(actual);
    list_add_tail(&actual->list,&freequeue);
    actual->PID = -1;
    actual->dir_pages_baseAddr = NULL;
    actual->kernel_esp = NULL;
    actual->quantum = NULL;
    //update_process_state_rr(actual, &freequeue);
    sched_next_rr();
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
