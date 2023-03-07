/*
 * interrupt.h - Definició de les diferents rutines de tractament d'exepcions
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>

#define IDT_ENTRIES 256

extern Gate idt[IDT_ENTRIES];
extern Register idtR;
void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL);
void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL);

void setIdt();

extern int zeos_ticks;

/* Definición de handlers */
void clock_handler(void);
void keyboard_handler(void);
void pf_handler(void);
void syscall_handler_sysenter(void);

#endif  /* __INTERRUPT_H__ */
