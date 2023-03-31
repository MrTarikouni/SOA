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


/* Buscamos las páginas en las que mapear las páginas lógicas del hijo */
int allocate_frames(int *frames[], struct task_struct *ts_hijo) {
    for (int i = 0; i < NUM_PAG_DATA; ++i) {
        if (frames[i]=alloc_frame() < 0) {
            for (int j = 0; j < i; ++j) free_frame(frames[j]);
            list_add(&ts_hijo->list, &freequeue);
            return -ENOMEM;
        }//
    }
    return 1;
}

/* Copia la user data+stak del padre al hijo */
void copy_pag_data(page_table_entry *TP_padre, page_table_entry *TP_hijo) {
    for (int i = 0; i < NUM_PAG_DATA; ++i) {
        /* mapeamos la página física del hijo en la página lógica del padre de la región vacía de la TP */
        set_ss_pag(TP_padre, i+PAG_LOG_INIT_CODE+NUM_PAG_CODE, get_frame(TP_hijo, i+PAG_LOG_INIT_DATA));
        /* copiamos los datos con los mapeos que tenemos en la TP del padre */
        copy_data((i+PAG_LOG_INIT_DATA << 12), (i+PAG_LOG_INIT_CODE+NUM_PAG_CODE << 12), PAGE_SIZE);
        /* liberamos la página lógica de la TP del padre que mapeaba la página física del hijo */
        del_ss_pag(TP_padre, i+PAG_LOG_INIT_CODE+NUM_PAG_CODE);
    }

}

unsigned int pid_count = 1;

int sys_fork()
{
  struct task_struct* pcb_hijo;
  // Retornamos error si la freequeue está vacía
  if (list_empty(&freequeue)) return -EFAULT;
  pcb_hijo = list_head_to_task_struct(list_first(&freequeue));
  list_del(&pcb_hijo->list);

  // Asignamos TP al hijo
  if (allocate_DIR(pcb_hijo) == -1) {
      list_add(&pcb_hijo->list, &freequeue);
      return -EFAULT;
  }
  // Copiamos el PCB del padre al hijo
  copy_data(current(),pcb_hijo,sizeof(union task_union));

  union task_union *tu_hijo = (union task_union*) pcb_hijo;

  // Páginas físicas  padre de user data+stack
  int frames[NUM_PAG_DATA], res;
  if (res = allocate_frames(&frames, tu_hijo) < 0) return res;

  page_table_entry *TP_padre = get_PT(current()), *TP_hijo = get_PT(&pcb_hijo);
  // Mismas direcciones del KERNEL de la TP del hijo y del padre
  for (int i = 0; i < NUM_PAG_KERNEL; ++i)
      set_ss_pag(TP_hijo, i, get_frame(TP_padre,i));
  // Las direcciones del código de usuario también se comparten
  for (int i = PAG_LOG_INIT_CODE; i < NUM_PAG_CODE; ++i)
      set_ss_pag(TP_hijo, i, get_frame(TP_padre,i));


  // Entradas de la user data+stack que apuntan a las NUM_PAG_DATA
  for (int i = PAG_LOG_INIT_DATA; i < NUM_PAG_DATA; ++i)
      set_ss_pag(TP_hijo, i, frames[i]);

  copy_pag_data(&TP_padre, &TP_hijo);

  // Flush TLB para que el padre no las traducciones del hijo
  set_cr3(get_DIR(current()));
  // Asignamos un PID=PID del anterior proceso creado +1
  pcb_hijo->PID=++pid_count;

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
           -------------/
*/

  tu_hijo->stack[1005]=0;
  tu_hijo->stack[1006]=&ret_from_fork;
  pcb_hijo->kernel_esp=tu_hijo->stack[1005];

  // añadimos al hijo en la readyqueue
  list_add_tail(&pcb_hijo->list, &readyqueue);

  return pcb_hijo->PID;
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
