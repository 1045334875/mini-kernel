// trap.c 
#include "sbi.h"
#include "printk.h"
#include "proc.h"
#include "defs.h"
#include "syscall.h"

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs ) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略
    //printk("trap_handler scause:%lx\n",scause);
    unsigned long a = 0x8000000000000005;
    if (scause == 0x8000000000000005) {
        printk("[S] Supervisor Mode Timer Interrupt\n");
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
        else if(regs->a7 == SYS_CLONE){
            regs->a0 = clone(regs);
            
        }
        regs->sepc =(unsigned long)((unsigned long)regs->sepc) + (unsigned long)4;
        
    }
    else if(scause == 12 || scause == 13 || scause == 15){
        do_page_fault(regs, scause);
    }
    return ;
}

extern char uapp_start[];
extern char uapp_end[];
extern struct task_struct *current;
void do_page_fault(struct pt_regs *regs, unsigned long scause) {
    /*
    1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
    2. 通过 scause 获得当前的 Page Fault 类型
    3. 通过 find_vm() 找到对应的 vm_area_struct
    4. 通过 vm_area_struct 的 vm_flags 对当前的 Page Fault 类型进行检查
        4.1 Instruction Page Fault      -> VM_EXEC
        4.2 Load Page Fault             -> VM_READ
        4.3 Store Page Fault            -> VM_WRITE
    5. 最后调用 create_mapping 对页表进行映射
    */
    uint64 bad_address = csr_read(stval);
    uint64 page_fault_kind = scause;
    int term=0b10001;//valid
    printk("Do Page fault:%d bad address = %lx\n",scause, bad_address);
    uint64 a = csr_read(satp);
    uint64 *pgtbl;
    pgtbl = ((unsigned long)current->pgd+ PA2VA_OFFSET);
    //printk("pgd:%lx  phy pgd = %lx\n",pgtbl,(unsigned long)pgtbl - PA2VA_OFFSET);
    struct vm_area_struct *temp = find_vma(current->mm, bad_address);
    if(temp == NULL){
        do_mmap(current->mm, bad_address, PGSIZE, 0);
        temp = current->mm->mmap;
    }
    else {
        term |= (temp->vm_flags << 1);
    }
    switch(page_fault_kind){
        case 12: temp->vm_flags |= VM_EXEC;  term |= 0b1000; break;//readable
        case 13: temp->vm_flags |= VM_READ;  term |= 0b0010; break;//writable
        case 15: temp->vm_flags |= VM_WRITE; term |= 0b0100; break;//executable
        default: break;
    }
    
    
    if(bad_address >= USER_START && bad_address < (USER_START + (unsigned long)uapp_end -  (unsigned long)uapp_start)){
        create_mapping(pgtbl, temp->vm_start, uapp_start - PA2VA_OFFSET, (unsigned long)uapp_end -  (unsigned long)uapp_start, 0b11111);
        //printk("direct linear map :%d bad address = %lx\n",scause, bad_address);
    }
    else if(temp->vm_start == USER_END - PGSIZE){
        unsigned long pa = current->user_sp - PGSIZE; // virtual address
        create_mapping(pgtbl, temp->vm_start, pa - PA2VA_OFFSET, PGSIZE, 0b10111);
    }
    else{
        uint64 pa = kalloc();//虚拟地址
        create_mapping(pgtbl, temp->vm_start, pa - PA2VA_OFFSET, PGSIZE, term);
    }
    //create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) 
}
