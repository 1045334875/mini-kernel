//arch/riscv/kernel/proc.c
#include "proc.h"
#include "defs.h"
#include "string.h"
#include "mm.h"
#include "printk.h"
#include "sbi.h"

extern void __dummy();
extern unsigned long swapper_pg_dir[512];
extern char uapp_start[];
extern char uapp_end[];
struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此


void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 taks[0] 指向 idle
    idle = kalloc();
    idle->state = TASK_RUNNING;
    idle->counter=0;
    idle->priority=0;
    idle->pid=0;
    current=idle;
    task[0]=idle;
    
    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0,
    // priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`, 
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， 
    //`sp` 设置为 该线程申请的物理页的高地址
    for(int i = 1; i < NR_TASKS; i++) {
            task[i] = (struct task_struct *)kalloc(); 
            task[i]->state =  TASK_RUNNING;
            task[i]->counter = 0;
            task[i]->priority = rand();
            task[i]->pid = i;
            task[i]->thread.ra = __dummy;
            task[i]->thread.sp = (uint64)task[i] + PGSIZE;

            //set SUM(bit 18, so kernel mode can access user mode page), 
            //set SPIE(bit 5, so interruption is enabled after sret), 
            //set SPP to be 0, so after calling mret, the system can return to user mode 
            task[i]->thread.sstatus =  csr_read(sstatus);
            task[i]->thread.sstatus = task[i]->thread.sstatus | 0x00040020; 
            csr_write(sstatus, task[i]->thread.sstatus); 

            task[i]->thread.sepc =  USER_START;
            task[i]->thread.sscratch = USER_END;

            
            unsigned long * user_stack = kalloc();//用户栈的虚拟地址
            unsigned long* rootPGT = kalloc();////页表的虚拟地址 
            
            task[i]->pgd = (unsigned long)rootPGT - PA2VA_OFFSET;//物理地址
            for (int i = 0; i < 512; i++)
            {
                rootPGT[i] =  swapper_pg_dir[i];
            }

            //create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
            create_mapping(rootPGT,  USER_END-PGSIZE ,  (unsigned long )user_stack - PA2VA_OFFSET,  PGSIZE , 0b10111);
            create_mapping(rootPGT,  USER_START ,  (unsigned long)uapp_start-PA2VA_OFFSET, (unsigned long)uapp_end -  (unsigned long)uapp_start, 0b11111);
    }
    printk("...proc_init done!\n");
}


void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d. thread space begin at 0x%lx\n", current->pid, auto_inc_local_var,current); 
        }
    }
}

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

void switch_to(struct task_struct* next) {
    if(next!=current) {
        //printk("switch to [PID = %d COUNTER = %d PRIORITY = %d]\n",next->pid,next->counter,next->priority); 
        struct task_struct* temp=current;
        current=next;
        __switch_to(temp,next);
        
    }
}

void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 或者 当前线程运行剩余时间为0 进行调度 */
    /* 2. 如果当前线程不是 idle 且 运行剩余时间不为0 则对当前线程的运行剩余时间减1 直接返回 */
    
    if(current!=idle&&current->counter!=0){
        current->counter--;
    }
    if(current==idle||current->counter==0){
        schedule();
    }
}


//短作业优先调度
#ifdef SJF

void schedule(void) {
	//printk("Enter SJF schedule\n");
	
    int scheMin=999999;
    struct task_struct* next;
    next=task[0];
    
    while(1){
        for(int i=1;i<NR_TASKS;i++){
         // printk("look [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter); 
            if(task[i]->state==TASK_RUNNING &&task[i]->counter&& (int)task[i]->counter < scheMin){
                scheMin= task[i]->counter ;
                next=task[i];
            }
        }
        if(next==idle){
            for(int i=1;i<NR_TASKS;i++){
                task[i]->counter=rand();
                printk("SET [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter); 
            }
        }
        else 
            break;
    }
   
    switch_to(next);
}
#endif


//优先级调度
#ifdef PRIORITY

void schedule(void) {
    int c,pri=0;
    struct task_struct* next;
    while(1){
        c=-1;
        next=task[0];
        for(int i=1;i<NR_TASKS;i++){
            if(task[i]->state==TASK_RUNNING && (int)task[i]->counter){
                if(task[i]->priority>next->priority){
                    next=task[i];
                }
            }
        }
        if(next==task[0]){
            for(int i=1;i<NR_TASKS;i++){
                task[i]->counter = rand();
                printk("SET [PID = %d COUNTER = %d PRIORITY = %d]\n",task[i]->pid,task[i]->counter,task[i]->priority); 
            }
            printk("\n");
            
        }
        else
            break;
    }
    switch_to(next);
}
#endif

