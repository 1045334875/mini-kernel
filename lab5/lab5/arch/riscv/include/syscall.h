#ifndef SYSCALL_H
#define SYSCALL_H
#include "types.h"

int sys_getpid();
void sys_write(unsigned int fd, const char* buf, unsigned int count);

struct pt_regs {
    unsigned long sstatus;
    unsigned long sepc;
    unsigned long t6, t5, t4, t3, s11, s10, s9, s8, s7, s6, s5, s4, s3, s2;
    unsigned long a7, a6, a5, a4, a3, a2, a1, a0, s1, s0, t2, t1, t0;
    unsigned long tp, gp, sp, ra, x0;

};

#endif