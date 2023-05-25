/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define BUFFER_SIZE 256

extern char circular_buffer[BUFFER_SIZE];
extern char* read_pointer;
extern char* write_pointer;
extern int items;

extern int fg,bg;
extern int x,y;

void * get_ebp();

int check_fd(int fd, int permissions)
{
    if (fd!=1) return -EBADF; 
    if (permissions!=ESCRIPTURA) return -EACCES; 
    return 0;
}

void user_to_system(void)
{
    update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
    update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
    return -ENOSYS; 
}

int sys_getpid()
{
    return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
    return 0;
}

//returns the index of the frame in the frame_pool
int find_shared_frame(int frame) {
    for (int i = 0; i < 10; ++i) {
         if (frame_pool[i].id_frame == frame) return i;
    }
    return -1;//shoud never happen
}

int sys_fork(void)
{
    struct list_head *lhcurrent = NULL;
    union task_union *uchild;

    /* Any free task_struct? */
    if (list_empty(&freequeue)) return -ENOMEM;

    lhcurrent=list_first(&freequeue);

    list_del(lhcurrent);

    uchild=(union task_union*)list_head_to_task_struct(lhcurrent);

    /* Copy the parent's task struct to child's */
    copy_data(current(), uchild, sizeof(union task_union));

    /* new pages dir */
    allocate_DIR((struct task_struct*)uchild);

    /* Allocate pages for DATA+STACK */
    int new_ph_pag, pag, i;
    page_table_entry *process_PT = get_PT(&uchild->task);
    for (pag=0; pag<NUM_PAG_DATA; pag++)
    {
        new_ph_pag=alloc_frame();
        if (new_ph_pag!=-1) /* One page allocated */
            {
                set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
            }
        else /* No more free pages left. Deallocate everything */
        {
            /* Deallocate allocated pages. Up to pag. */
            for (i=0; i<pag; i++)
            {
                free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
                del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
            }
            /* Deallocate task_struct */
            list_add_tail(lhcurrent, &freequeue);

            /* Return error */
            return -EAGAIN; 
        }
    }

    /* Copy parent's SYSTEM and CODE to child. */
    page_table_entry *parent_PT = get_PT(current());
    for (pag=0; pag<NUM_PAG_KERNEL; pag++)
    {
        set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
    }
    for (pag=0; pag<NUM_PAG_CODE; pag++)
    {
        set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
    }
    /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
    /*for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
    {
        
        set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
        copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
        del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
    }*/

    for (pag=PAG_LOG_INIT_DATA; pag<PAG_LOG_INIT_DATA+NUM_PAG_DATA; pag++)
    {
        set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
        parent_PT[pag].bits.rw = 0;
        process_PT[pag].bits.rw = 0;
    }

    //Copy shared pages from parent to child
    for (pag=PAG_LOG_INIT_DATA+2*NUM_PAG_DATA; pag<TOTAL_PAGES; ++pag) {
        int frame = get_frame(parent_PT, pag);
        if (frame != 0) {
            int index_frame = find_shared_frame(frame);
            frame_pool[index_frame].num_ref++;
            set_ss_pag(process_PT, pag,frame);
        }
    }
    /* Deny access to the child's memory space */
    set_cr3(get_DIR(current()));

    uchild->task.PID=++global_PID;
    uchild->task.state=ST_READY;

    int register_ebp;		/* frame pointer */
    /* Map Parent's ebp to child's stack */
    register_ebp = (int) get_ebp();
    register_ebp=(register_ebp - (int)current()) + (int)(uchild);

    uchild->task.register_esp=register_ebp + sizeof(DWord);

    DWord temp_ebp=*(DWord*)register_ebp;
    /* Prepare child stack for context switch */
    uchild->task.register_esp-=sizeof(DWord);
    *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
    uchild->task.register_esp-=sizeof(DWord);
    *(DWord*)(uchild->task.register_esp)=temp_ebp;

    /* Set stats to 0 */
    init_stats(&(uchild->task.p_stats));

    /* Queue child process into readyqueue */
    uchild->task.state=ST_READY;
    list_add_tail(&(uchild->task.list), &readyqueue);

    return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
    char localbuffer [TAM_BUFFER];
    int bytes_left;
    int ret;

    if ((ret = check_fd(fd, ESCRIPTURA)))
        return ret;
    if (nbytes < 0)
        return -EINVAL;
    if (!access_ok(VERIFY_READ, buffer, nbytes))
        return -EFAULT;

    bytes_left = nbytes;
    while (bytes_left > TAM_BUFFER) {
        copy_from_user(buffer, localbuffer, TAM_BUFFER);
        ret = sys_write_console(localbuffer, TAM_BUFFER);
        bytes_left-=ret;
        buffer+=ret;
    }
    if (bytes_left > 0) {
        copy_from_user(buffer, localbuffer,bytes_left);
        ret = sys_write_console(localbuffer, bytes_left);
        bytes_left-=ret;
    }
    return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
    return zeos_ticks;
}

void sys_exit()
{  
    int i;

    page_table_entry *process_PT = get_PT(current());

    // Deallocate all the propietary physical pages
    for (i=0; i<NUM_PAG_DATA; i++)
    {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
    }

    // Deallocate the shared physical pages
    int frame;
    for (i=PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA; i<TOTAL_PAGES; i++) {
        frame = get_frame(process_PT, i);
        if (frame != 0) {
            int index_frame = find_shared_frame(frame);
            frame_pool[index_frame].num_ref--; 
            del_ss_pag(process_PT, i);
            if (frame_pool[index_frame].num_ref == 0 && frame_pool[index_frame].delete) {//its not referenced anymore               //clear page starting at addr
                for (int i = 0; i < PAGE_SIZE; ++i) {
                    *((char*) (frame << 12) + i) = 0;
                }
            }
        }
    }
        
    /* Free task_struct */
    list_add_tail(&(current()->list), &freequeue);

    current()->PID=-1;

    /* Restarts execution of the next process */
    sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
    force_task_switch();
    return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
    int i;

    if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 

    if (pid<0) return -EINVAL;
    for (i=0; i<NR_TASKS; i++)
    {
        if (task[i].task.PID==pid)
        {
            task[i].task.p_stats.remaining_ticks=remaining_quantum;
            copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
            return 0;
        }
    }
    return -ESRCH; /*ESRCH */
}

int sys_read(char* b, int maxchars){
    int i = 0;
    if (maxchars < 0) return -EINVAL;
    if (!access_ok(VERIFY_READ,b, sizeof(int)*maxchars)) return -EFAULT;
    while ((i < maxchars || i < items) && read_pointer != write_pointer) {
        //b* = *read;
        if (copy_to_user(read_pointer,b,sizeof(char)) < 0) return -EFAULT;
        b++;  
        read_pointer++;
        //read %= BUFFER_SIZE;
        if (read_pointer == &circular_buffer[BUFFER_SIZE]) read_pointer = &circular_buffer[0];
        i++;
        --items;
    }
    return i;
}

// Changes the current position of the cursor to the x column and y row
int sys_gotoxy(int mx, int my) {
    if (mx < 0 || mx > NUM_COLUMNS || my < 0 || my > NUM_ROWS) return -1;
	x=mx;
	y=my;
	return 0;

}

int sys_set_color(int foreground, int background){
    if (foreground < 16 && foreground>= 0 && background<16 && background >= 0){
        fg = foreground;
        bg = background;
        return 0;
    }
    return -EINVAL;
}

int find_empty_page(){
    for (int i = PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA; i < TOTAL_PAGES; ++i) {
        if (!get_frame(get_PT(current()), i)) return i;
    }
    return -1;
}

int valid_addr(int *addr) {
    return (addr != NULL && !get_frame(get_PT(current()), (int) addr >> 12) && ((PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA) <= ((int) addr >> 12)/*fuera de la zona del fork*/));
}

int sys_shmat(int id, void* addr) {
    if (id < 0 || id > 9) return -EINVAL;
    if ((int) addr % PAGE_SIZE != 0) return -EINVAL;//addr must be page aligned
    int free_page;
    if (!valid_addr(addr)) {
        if ((free_page = find_empty_page()) < 0) return -EFAULT;
    }
    else free_page = (int) addr >> 12;
    
    set_ss_pag(get_PT(current()), free_page, frame_pool[id].id_frame);
    frame_pool[id].num_ref++;//inc ref count

    return free_page;
}

//Remove the shared region at logical address 'addr'
int sys_shmdt(void* addr) {
    if ((int) addr % PAGE_SIZE != 0) return -EINVAL;//addr must be page aligned
    int frame;
    if ((frame = get_frame(get_PT(current()), (int) addr >> 12)) == 0) return -EFAULT;
    int index_frame = find_shared_frame(frame);
    frame_pool[index_frame].num_ref--;
    if (frame_pool[index_frame].num_ref == 0 && frame_pool[index_frame].delete) {//its not referenced anymore
        //clear page starting at addr
        for (int i = 0; i < PAGE_SIZE; ++i) {
            *((char*) addr + i) = 0;
        }
    }
    del_ss_pag(get_PT(current()), (int) addr >> 12);
    return 0;
}

int sys_shmrm(int id) {
    if (id < 0 || id > 9) return -EINVAL;
    frame_pool[id].delete = 1;
    return 0;
}

