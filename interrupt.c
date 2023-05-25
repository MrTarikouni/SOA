/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <sched.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

extern Byte phys_mem[TOTAL_PAGES];

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

int zeos_ticks = 0;

void clock_routine()
{
  zeos_show_clock();
  zeos_ticks ++;
  
  schedule();
}

#define BUFFER_SIZE 256

char circular_buffer[BUFFER_SIZE];
char* read_pointer = &circular_buffer[0];
char* write_pointer = &circular_buffer[0];
int items = 0;

void keyboard_routine()
{
  unsigned char c = inb(0x60);
  
  
  if (items < BUFFER_SIZE && c&0x80){
    *write_pointer = char_map[c&0x7f];
    write_pointer++;
    if (write_pointer == &circular_buffer[BUFFER_SIZE]) write_pointer = &circular_buffer[0];
    //write = write % BUFFER_SIZE;
    items++;
  }
  
}

void pf_routine(int flags, int eip) {
  char bff[10];
  itoa(eip,bff);
  //printk("PAGE FAULT exception at EIP: 0x");
  int quotient, remainder;
  int i, j = 0;

  char tmp[100];

  quotient = eip;

  while (quotient != 0)   //Decimal a hexadecimal
  {
      remainder = quotient % 16;
      if (remainder < 10)
          tmp[j++] = 48 + remainder;
      else
          tmp[j++] = 55 + remainder;
      quotient = quotient / 16;
  }

  page_table_entry *PT = get_PT(current());
  int logical_page = (int) get_cr2() >> 12;
  int physical_page = get_frame(PT,logical_page);
  if (phys_mem[physical_page] >1){
    if (PT[logical_page].bits.rw == 0){
    int new_ph_pag = alloc_frame();
    int pag;
    for (pag = PAG_LOG_INIT_DATA + NUM_PAG_DATA; pag < TOTAL_PAGES && PT[pag].bits.present == 1; ++pag); 
    set_ss_pag(PT, pag, new_ph_pag);
    copy_data((void*)(logical_page << 12),(void*)(pag << 12), PAGE_SIZE);
    del_ss_pag(PT,logical_page);
    set_ss_pag(PT,logical_page,new_ph_pag);

    del_ss_pag(PT,pag);
    }
  }
  else if (phys_mem[physical_page] == 1){
    PT[logical_page].bits.rw = 1;
  }
  else {
    for (int w = j; 8-w != 0; ++w) printc('0');
  for (i = j-1; i >= 0; i--) printc(tmp[i]);
  }

  
  //while(1);
}


void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void clock_handler();
void keyboard_handler();
void system_call_handler();
void pf_handler();

void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void setSysenter()
{
  setMSR(0x174, 0, __KERNEL_CS);
  setMSR(0x175, 0, INITIAL_ESP);
  setMSR(0x176, 0, (unsigned long)system_call_handler);
}

void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(14, pf_handler, 0);
  setSysenter();

  set_idt_reg(&idtR);
}

