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
struct list_head freequeue;      /* Freequeue */
struct list_head readyqueue;     /* Readyqueue */

int current_quantum;

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
    printk("\nESTOY EN IDLE");
    __asm__ __volatile__("sti": : :"memory");
	while(1)
	{
	;
	}
}

struct task_struct *idle_task;

void init_idle (void)
{
  idle_task = list_head_to_task_struct(list_first(&freequeue));		// Cogemos un task struct de la freequeue
  union task_union *tu_idle = (union task_union*)idle_task;

  list_del(&idle_task->list);						// Borramos el task union de la freequeue

  idle_task->PID = 0;
  idle_task->quantum = DEFAULT_QUANTUM;
  allocate_DIR(idle_task);						// Alocatamos el directorion de la tabla de páginas



  /* |           |
   * |           |
   * |-----------|
   * |	   0	 | <----- KERNEL_ESP
   * |-----------|
   * | @cpu_idle |
   * -------------
   * */
  tu_idle->stack[1023]= (unsigned long)&cpu_idle;
  tu_idle->stack[1022]= 0;
  idle_task->kernel_esp= (unsigned long)&(tu_idle->stack[1022]);
}

struct task_struct *init_task;

void init_task1(void)
{
  init_task = list_head_to_task_struct(list_first(&freequeue));       		// Cogemos un task struct de la freequeue
  union task_union *tu_init = (union task_union*)init_task;

  list_del(&init_task->list);                      		                // Borramos el task union de la freequeue

  init_task->PID=1;
  init_task->PID=DEFAULT_QUANTUM;
  allocate_DIR(init_task);

  set_user_pages(init_task);

  tss.esp0=(unsigned long)&tu_init->stack[1024];

  writeMSR(0x175,0x0, &tu_init->stack[1024]);

  set_cr3(get_DIR(init_task));
}

void init_sched()
{
	INIT_LIST_HEAD(&freequeue);			//Inicializar la freequeue vacia
	INIT_LIST_HEAD(&readyqueue); 			//Inicializar la readyqueue vacía
	for (int i = 0; i < NR_TASKS; ++i) 		//Inicializar la freequeue con todos los task_structs.
		list_add_tail(&(task[i].task.list),&freequeue);
}


void inner_task_switch(union task_union*t){
    printk("\nCAMBIANDO PROCESO");
	page_table_entry *dir = get_DIR(&t->task);
    tss.esp0 = (unsigned long)&t->stack[1024];
    writeMSR( 0x175, 0x0,(unsigned long)&t->stack[1024]);
    set_cr3(dir);
	switch_context(&current()->kernel_esp, t->task.kernel_esp);
}

int get_quantum (struct task_struct *t) {
    return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum) {
    t->quantum = new_quantum;
}

void update_sched_data_rr() {
    --current_quantum;
}


int needs_sched_rr () {
    if (current_quantum > 0) return 0;
    else if (list_empty(&readyqueue)) {
        current_quantum = current()->quantum;
        return 0;
    }
    return 1;
}

void update_process_state_rr (struct task_struct *t, struct list_head *dst_queue) {
    if (t->st!=ST_RUN) list_del(&t->list); // El estado es ready o blocked
    if (dst_queue != NULL) { // La cola de destino es ready o block
        list_add_tail(&t->list, dst_queue);
        if (dst_queue == &readyqueue) t->st=ST_READY;
        else t->st=ST_BLOCKED;
    }
    else t->st=ST_RUN;
}

void sched_next_rr () {
    struct task_struct *next;
    if (!list_empty(&readyqueue)) {
        next=list_head_to_task_struct(list_first(&readyqueue));
        update_process_state_rr(next, NULL);
    }
    else next=idle_task;

    // update_process_state_rr(current(),?);

    current_quantum=get_quantum(next);
    task_switch((union task_union *)next);

}

void schedule () {
    update_sched_data_rr();
    if (needs_sched_rr()) {
        printk("\nREADYQUEUE VACIA");
        update_process_state_rr(current(),&readyqueue);
        sched_next_rr();
    }
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
