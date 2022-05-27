//arch/riscv/kernel/proc.c
#include "proc.h"
#include "defs.h"
#include "string.h"
#include "mm.h"
#include "printk.h"
#include "syscall.h"
#include "sbi.h"

extern void ret_from_fork(struct pt_regs *trapframe);
extern void __dummy();
extern unsigned long swapper_pg_dir[512];
extern char uapp_start[];
extern char uapp_end[];
struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

int total_task;

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr){
    if(mm == NULL) return NULL;
    struct vm_area_struct *temp = mm->mmap;
    if(temp == NULL) return NULL;
    while(temp->vm_next){
        if(temp->vm_start <= addr && temp->vm_end > addr){
            return temp;
        }
        temp = temp->vm_next;
    }
    if(temp->vm_start <= addr && temp->vm_end > addr){
            return temp;
    }
    else return NULL;
}

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot){
    printk("Do_mmap: start addr:%lx  length:%lx\n",addr, length);
        
    struct vm_area_struct *newnode = (struct vm_area_struct *)kalloc();
    newnode->vm_prev = NULL;
    newnode->vm_next = NULL;
    newnode->vm_flags = prot;
    newnode->vm_mm = mm;
    struct vm_area_struct *temp = mm->mmap;
    int flag = 0;
    if(temp == NULL){
        mm->mmap = newnode;
        newnode->vm_start = addr;
        newnode->vm_end = addr + length;
        //printk("Do_mmap: start addr:%lx  end addr:%lx\n",newnode->vm_start, newnode->vm_end);
        return addr;
    }
    if(temp->vm_start > addr){//添加在链表最前面
        if(temp->vm_start <= addr + length){//页表前面没有空间，那就得放后面
            addr = get_unmapped_area(mm,length);
            flag = 1;
        }
        else{//页表前面有空间
            temp->vm_prev = newnode;
            newnode->vm_next = temp;
            mm->mmap = newnode;

            newnode->vm_start = addr;
            newnode->vm_end = addr + length;
            //printk("Do_mmap: start addr:%lx  end addr:%lx\n",newnode->vm_start, newnode->vm_end);
            return addr;
        }
    }
    while(temp->vm_next){           //addr必然在当前块start后面，再分end前、后讨论
        if(addr < temp->vm_end){   //如果addr在当前vma-end的前面，那必然冲突了
            if(!flag) addr = get_unmapped_area(mm, length);
            break;
        }
        if(addr >= temp->vm_end && addr < temp->vm_next->vm_start){ //如果addr在当前vma和下一个vma之间空挡
            if(temp->vm_next->vm_start < addr + length) //如果本次length会覆盖到下一个vma
                if(!flag) addr = get_unmapped_area(mm, length);
            break;//本次length不会覆盖下一个vma

        }
        temp=temp->vm_next;
    }
    //写着写着发现这里不需要映射，等到pagefault的时候才映射
    //create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
    //create_mapping(current->pgd, addr, phy_addr, length, prot);
    
    newnode->vm_start = addr;
    newnode->vm_end = addr + length;
    if(temp->vm_next) temp->vm_next->vm_prev = newnode;
    newnode->vm_next = temp->vm_next;
    newnode->vm_prev = temp;
    temp->vm_next = newnode;
    //printk("Do_mmap: start addr:%lx  end addr:%lx\n",newnode->vm_start, newnode->vm_end);
    return addr;
}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length){
    uint64 *i, *j;
    //遍历vma链表寻找之中空的那个
    //printk("get unmapped area length %x\n",length);
    struct  vm_area_struct *temp = mm->mmap;
    //printk("mmap %lx - %lx\n",temp->vm_start, temp->vm_end);
    uint64 addr=0;
    if(length <= temp->vm_start) return addr;
    while(temp->vm_next){
        //printk("mmap %lx - %lx\n",temp->vm_start, temp->vm_end);
        if(temp->vm_next->vm_start - temp->vm_end >= length){
            return temp->vm_end;
        }
        temp = temp->vm_next;
    }
    return temp->vm_end;
}

printfram(const struct pt_regs *regs){
    printk("cur pid: %lx sp: %lx sscratch:%lx\n",current->pid,current->thread.sp,current->thread.sscratch);
    printk("regs sp %lx sscratch: %lx \n",regs->sp, csr_read(sscratch));
    printk("sepc:%lx  sstatus: %lx\n",regs->sepc, regs->sstatus);
    printk("a0:%lx a1: %lx\n",regs->a0, regs->a1);
    printk("a2:%lx a3: %lx\n",regs->a2, regs->a3);
    printk("a4:%lx a5: %lx\n",regs->a4, regs->a5);
    printk("a6:%lx a7: %lx\n",regs->a6, regs->a7);
    printk("t0:%lx t1: %lx\n",regs->t0, regs->t1);
    printk("t2:%lx t3: %lx\n",regs->t2, regs->t3);
    printk("s0:%lx s1: %lx\n",regs->s0, regs->s1);
    printk("s2:%lx s3: %lx\n",regs->s2, regs->s3);
    printk("ra:%lx gp: %lx\n",regs->ra, regs->gp);
}


void forkret() {
    //printfram(current->trapframe);
    ret_from_fork(current->trapframe);
}

uint64 do_fork(struct pt_regs *regs) {
    
    int sum=total_task, i=total_task;
    
    //printk("Enter do_fork\ntotal_task = %d\n",total_task);
    task[i] = (struct task_struct *)kalloc(); 
    task[i]->state =  TASK_RUNNING;
    task[i]->counter = 0;
    task[i]->priority = rand();
    task[i]->pid = total_task;
    task[i]->thread.ra = (uint64)forkret;
    task[i]->thread.sp = (uint64)task[i] + PGSIZE;//s-MODE stack
    task[i]->mm = (struct mm_struct *)kalloc();
    task[i]->mm->mmap = NULL;
    //printk("[PID = %d] fork [PID = %d]\n",current->pid, task[i]->pid);
    
    task[i]->trapframe = (struct pt_regs *)kalloc();
    //set SUM(bit 18, so kernel mode can access user mode page), 
    //set SPIE(bit 5, so interruption is enabled after sret), 
    //set SPP to be 0, so after calling mret, the system can return to user mode 
    
    task[i]->thread.sstatus =  csr_read(sstatus);
    task[i]->thread.sstatus = task[i]->thread.sstatus | 0x00040020; 
    csr_write(sstatus, task[i]->thread.sstatus); 

    task[i]->thread.sepc =  regs->sepc;//父进程ecall时的pc
    task[i]->thread.sscratch = (uint64)task[i] + PGSIZE;

    total_task++;
    unsigned long * user_stack = (unsigned long *)kalloc();//U-MODE栈的虚拟地址
    //printk("pid : %lx  s-stack = %lx\n",task[i]->pid, user_stack);
    task[i]->user_sp = (uint64)user_stack + PGSIZE;//u-MODE stack
    
    for(int j=0; j<512; j++){
        user_stack[j] = ((unsigned long*)(USER_END-PGSIZE))[j]; //拷贝用户栈的内容
        //printk("stack addr %lx  data %lx\n",&user_stack[j], user_stack[j]);
    }
    
    unsigned long* rootPGT = (unsigned long *)kalloc();////页表的虚拟地址 
    //printk("rootPGT1  %lx  user_stack=%lx  \n",(unsigned long)rootPGT,user_stack);
    task[i]->pgd = (unsigned long)((unsigned long)rootPGT - (unsigned long)PA2VA_OFFSET);//物理地址
    for (int j = 0; j < 512; j++)
    {
        rootPGT[j] =  swapper_pg_dir[j];
    }
    
    //printk("mm - %lx\n",(unsigned long)task[i]->mm);
    for(struct vm_area_struct *mmapi = current->mm->mmap; mmapi; mmapi = mmapi->vm_next){
        //printk("copy mmap- %lx  size = %lx\n",(unsigned long)mmapi->vm_start, mmapi->vm_end - mmapi->vm_start);
        do_mmap(task[i]->mm, mmapi->vm_start, mmapi->vm_end - mmapi->vm_start, mmapi->vm_flags);
    }

    
    //printk("new trapframe %lx  regs %lx\n",task[i]->trapframe, regs);
    current->trapframe = (struct pt_regs *)regs;
    //copy trapframe 

    task[i]->trapframe->sepc = regs->sepc; 
    task[i]->trapframe->sstatus = regs->sstatus;
    task[i]->trapframe->t6 = regs->t6; 
    task[i]->trapframe->t5 = regs->t5; 
    task[i]->trapframe->t4 = regs->t4; 
    task[i]->trapframe->t3 = regs->t3;
    task[i]->trapframe->s11 = regs->s11; 
    task[i]->trapframe->s10 = regs->s10; 
    task[i]->trapframe->s9 = regs->s9; 
    task[i]->trapframe->s8 = regs->s8;
    task[i]->trapframe->s7 = regs->s7; 
    task[i]->trapframe->s6 = regs->s6; 
    task[i]->trapframe->s5 = regs->s5; 
    task[i]->trapframe->s4 = regs->s4;
    task[i]->trapframe->s3 = regs->s3; 
    task[i]->trapframe->s2 = regs->s2; 
    task[i]->trapframe->a7 = regs->a7; 
    task[i]->trapframe->a6 = regs->a6;
    task[i]->trapframe->a5 = regs->a5; 
    task[i]->trapframe->a4 = regs->a4; 
    task[i]->trapframe->a3 = regs->a3; 
    task[i]->trapframe->a2 = regs->a2;
    task[i]->trapframe->a1 = regs->a1; 
    task[i]->trapframe->a0 = regs->a0; 
    task[i]->trapframe->s1 = regs->s1; 
    task[i]->trapframe->s0 = regs->s0;
    task[i]->trapframe->t2 = regs->t2;   
    task[i]->trapframe->t1 = regs->t1;   
    task[i]->trapframe->t0 = regs->t0;   
    task[i]->trapframe->tp = regs->tp;
    task[i]->trapframe->gp = regs->gp;   
    task[i]->trapframe->sp = regs->sp;   
    task[i]->trapframe->ra = regs->ra;   
    task[i]->trapframe->x0 = regs->x0;
    //printk("parent user sp %lx sscratch %lx\n",current->thread.sp, current->thread.sscratch);
    //printk("parent_user_sp %lx  now sscratch %lx\n",current->user_sp, usersp);   
    task[i]->trapframe->sp = csr_read(sscratch);//修改为父进程的用户态sp
    task[i]->trapframe->a0 = 0;

    //printfram(task[i]->trapframe);
    return task[i]->pid;
}

uint64 clone(struct pt_regs *regs) {
    //printfram(regs);
    return do_fork(regs);
}


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
    total_task = 2;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0,
    // priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`, 
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， 
    //`sp` 设置为 该线程申请的物理页的高地址
    for(int i = 1; i < total_task; i++) {
        task[i] = (struct task_struct *)kalloc(); 
        task[i]->state =  TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = __dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;//s-MODE stack
        task[i]->mm = (struct mm_struct *)kalloc();
        task[i]->mm->mmap = NULL;
        //set i(bit 18, so kernel mode can access user mode page), 
        //set SPIE(bit 5, so interruption is enabled after sret), 
        //set SPP to be 0, so after calling mret, the system can return to user mode 
        task[i]->thread.sstatus =  csr_read(sstatus);
        task[i]->thread.sstatus = task[i]->thread.sstatus | 0x00040020; 
        csr_write(sstatus, task[i]->thread.sstatus); 

        task[i]->thread.sepc =  USER_START;
        task[i]->thread.sscratch = USER_END;

        
        unsigned long * user_stack = kalloc();//S-MODE栈的虚拟地址
        task[i]->user_sp = (uint64)user_stack + PGSIZE;//s-MODE stack
        unsigned long* rootPGT = kalloc();////页表的虚拟地址 
        //printk("rootPGT1  %lx  user_stack=%lx  \n",(unsigned long)rootPGT,user_stack);
        task[i]->pgd = (unsigned long)((unsigned long)rootPGT - (unsigned long)PA2VA_OFFSET);//物理地址
        //printk("rootPGT1 - %lx\n",(unsigned long)((unsigned long)task[i]->pgd + PA2VA_OFFSET));
        do_mmap(task[i]->mm, USER_END-PGSIZE, PGSIZE, VM_READ | VM_WRITE);//user stack
        do_mmap(task[i]->mm, USER_START, (unsigned long)uapp_end -  (unsigned long)uapp_start, VM_READ | VM_WRITE | VM_EXEC);

        
        for (int i = 0; i < 512; i++)
        {
            rootPGT[i] =  swapper_pg_dir[i];
        }
        /*
        //create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
        create_mapping(rootPGT,  USER_END-PGSIZE ,  (unsigned long )user_stack - PA2VA_OFFSET,  PGSIZE , 0b10111);
        create_mapping(rootPGT,  USER_START ,  (unsigned long)uapp_start-PA2VA_OFFSET, (unsigned long)uapp_end -  (unsigned long)uapp_start, 0b11111);
        */
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
        printk("switch to [PID = %d COUNTER = %d PRIORITY = %d]\n",next->pid,next->counter,next->priority); 
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


void schedule(void) {
	//printk("Enter SJF schedule\n");
	
    int scheMin=999999;
    struct task_struct* next;
    next=task[0];
    
    while(1){
        for(int i=1;i<total_task;i++){
         // printk("look [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter); 
            if(task[i]->state==TASK_RUNNING &&task[i]->counter&& (int)task[i]->counter < scheMin){
                scheMin= task[i]->counter ;
                next=task[i];
            }
        }
        if(next==idle){
            for(int i=1;i<total_task;i++){
                task[i]->counter=rand();
                printk("SET [PID = %d COUNTER = %d]\n",task[i]->pid,task[i]->counter); 
            }
        }
        else 
            break;
    }
   
    switch_to(next);
}



//优先级调度
#ifdef PRIORITYty

void schedule(void) {
    int c,pri=0;
    struct task_struct* next;
    while(1){
        c=-1;
        next=task[0];
        for(int i=1;i<total_task;i++){
            if(task[i]->state==TASK_RUNNING && (int)task[i]->counter){
                if(task[i]->priority>next->priority){
                    next=task[i];
                }
            }
        }
        if(next==task[0]){
            for(int i=1;i<total_task;i++){
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

