// trap.c 
#include "sbi.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs ) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略
    //printk("trap_handler scause:%x\n",scause);
    if (scause == 0x8000000000000005) {
        //printk("[S] Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
        do_timer();
    }  
    else if(scause == 8){
        if(regs->a7 == 64){//write
            sys_write(regs->a0, regs->a1, regs->a2);
        } 
        else if(regs->a7 == 172){//getpid
            regs->a0 = sys_getpid();
        }
        regs->sepc =((unsigned long)regs->sepc) + (unsigned long)4;
        
    }
    return ;
}