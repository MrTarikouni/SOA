/*
 * system.h - Capçalera del mòdul principal del sistema operatiu
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <types.h>


extern TSS         tss;
extern Descriptor* gdt;

void writeMSR(long int msr_reg,long int hi_val, long int lo_val);


#endif  /* __SYSTEM_H__ */
