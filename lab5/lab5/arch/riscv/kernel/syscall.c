#include "syscall.h"
#include "printk.h"
#include "types.h"
#include "mm.h"
#include "proc.h"

extern struct task_struct* current; 
void sys_write(unsigned int fd, const char* buf, unsigned int count){
    printk("%s", buf);
}

int sys_getpid(){
    return current->pid;
}
