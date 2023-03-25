/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct list_head *freequeue;      /* Freequeue */
struct list_head *readyqueue;     /* Readyqueue */

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

struct task_struct *idle_task;

void init_idle (void)
{
  struct task_struct *pcb = list_head_to_task_struct(&freequeue);	// Cogemos un task struct de la freequeue
  union task_union *tu = &pcb; 
 
  list_del(&freequeue);							// Borramos el task union de la freequeue

  pcb->PID = 0;
  allocate_DIR(pcb);				// Alocatamos el directorion de la tabla de páginas
  


  /* |           |
   * |           |
   * |-----------|
   * |	   0	 | <----- KERNEL_ESP
   * |-----------| 	       	
   * | @cpu_idle |
   * -------------
   * */
  tu->stack[1023]= &cpu_idle;
  tu->stack[1022]= 0;
  pcb->KERNEL_ESP= &tu->stack[1022];

  idle_task = pcb; 
}

void init_task1(void)
{
  struct task_struct *pcb = list_head_to_task_struct(&freequeue);       	// Cogemos un task struct de la freequeue
  union task_union *tu = &pcb;

  list_del(&freequeue);                                                 // Borramos el task union de la freequeue

  pcb->PID=1;
  allocate_DIR(pcb);

  set_user_pages(pcb);							

  tss.esp0=&tu->stack[1024];
  writeMSR(0x175,0x0, (unsigned long)&tu->stack[1024]);

  set_cr3(pcb->dir_pages_baseAddr);
}

void init_sched()
{	
	INIT_LIST_HEAD(freequeue);			//Inicializar la freequeue vacia
	INIT_LIST_HEAD(readyqueue); 			//Inicializar la readyqueue vacía
	for (int i = 0; i < NR_TASKS; ++i) 		//Inicializar la freequeue con todos los task_structs.
		list_add_tail(task[i].task.list,freequeue);
}

void switch_context(unsigned long* current_kernel_esp, unsigned long* new_kernel_esp);



void inner_task_switch(union task_union*t){

	page_table_entry *dir = get_DIR(&t->task);
	set_cr3(dir);
	tss.esp0 = t->stack[1024];
	writeMSR( 0x175, 0x0, t->stack[1024]);
	switch_context(current()->KERNEL_ESP, t->task.KERNEL_ESP);

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}
