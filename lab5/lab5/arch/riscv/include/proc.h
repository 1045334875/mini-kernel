// arch/riscv/include/proc.h

#include "types.h"

#define NR_TASKS (1 + 4) // 用于控制 最大线程数量 （idle 线程 + 31 内核线程）

#define TASK_RUNNING    0 // 为了简化实验，所有的线程都只有一种状态

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10
typedef unsigned long* pagetable_t;


/* vm_area_struct vm_flags */
#define VM_READ     0x00000001
#define VM_WRITE    0x00000002
#define VM_EXEC     0x00000004

#define SYS_CLONE 220


struct vm_area_struct {
    struct mm_struct *vm_mm;    /* The mm_struct we belong to. */
    uint64 vm_start;          /* Our start address within vm_mm. */
    uint64 vm_end;            /* The first byte after our end address 
                                    within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    struct vm_area_struct *vm_next, *vm_prev;

    uint64 vm_flags;      /* Flags as listed above. */
};

struct mm_struct {
    struct vm_area_struct *mmap;       /* list of VMAs */
};

/* 用于记录 `线程` 的 `内核栈与用户栈指针` */
struct thread_info {
    uint64 kernel_sp;
    uint64 user_sp;
};

/* 线程状态段数据结构 */
struct thread_struct {
    uint64 ra;
    uint64 sp;
    uint64 s[12];
    uint64 sepc;//
    uint64 sstatus;//控制信号，控制当前是否中断、S-Mode 可以访问 User 页面
    uint64 sscratch;// sscratch 设置为 U-Mode 的 sp，其值为 USER_END
};


/* 线程数据结构 */
struct task_struct {
    //struct thread_info* thread_info;//内存中是64位地址
    uint64 user_sp;//substitude 64bits
    uint64 state;    // 线程状态
    uint64 counter;  // 运行剩余时间 
    uint64 priority; // 运行优先级 1最低 10最高
    uint64 pid;      // 线程id

    struct thread_struct thread;
    pagetable_t pgd;//每个进程都有一个页表

    struct mm_struct *mm;

    struct pt_regs *trapframe;//fork
};

/* 线程初始化 创建 NR_TASKS 个线程 */ 
void task_init(); 

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer();

/* 调度程序 选择出下一个运行的线程 */
void schedule();

/* 线程切换入口函数*/
void switch_to(struct task_struct* next);

/* dummy funciton: 一个循环程序，循环输出自己的 pid 以及一个自增的局部变量*/
void dummy();

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr);


/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot);

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length);
