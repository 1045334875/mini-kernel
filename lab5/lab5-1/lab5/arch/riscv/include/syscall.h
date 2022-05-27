#ifndef SYSCALL_H
#define SYSCALL_H
#include "types.h"

int sys_getpid();
void sys_write(unsigned int fd, const char* buf, unsigned int count);

struct pt_regs {
    unsigned long x0,ra,sp,gp,tp;
    unsigned long t0,t1,t2,s0,s1,a0,a1,a2,a3,a4,a5,a6,a7;
    unsigned long s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,t3,t4,t5,t6;
    unsigned long sepc;
    unsigned long sstatus;
};

#endif