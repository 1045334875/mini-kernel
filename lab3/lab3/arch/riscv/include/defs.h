#ifndef _DEFS_H
#define _DEFS_H
#define PHY_START 0x0000000080000000
#define PHY_SIZE  128 * 1024 * 1024 // 128MB， QEMU 默认内存大小 
#define PHY_END   (PHY_START + PHY_SIZE)

#define PGSIZE 0x1000 // 4KB
#define PGNUM 0x200  //512
#define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1)))
#define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))

//sys3 lab3 new
#define OPENSBI_SIZE (0x200000)

#define VM_START (0xffffffe000000000)
#define VM_END   (0xffffffff00000000)
#define VM_SIZE  (VM_END - VM_START)

#define PA2VA_OFFSET (VM_START - PHY_START)
//new add end
#include "types.h"

#define csr_read(csr)                       \
({                                          \
    register uint64 __v;                    \
    __asm__ volatile("csrr" "%0," #csr      \
                        : :"r"(__v)         \
                        : "memory");        \
    __v;                                    \
})

#define csr_write(csr, val)                         \
({                                                  \
    uint64 __v = (uint64)(val);                     \
    asm volatile ("csrw " #csr ", %0"               \
                    : : "r" (__v)                   \
                    : "memory");                    \
})
#define WritePTE(__pte_addr,__ppn_to_write, __perm, __V)                                                                   \
{                                                                                                                                                                                               \
        *__pte_addr =   ((unsigned long)(*(__pte_addr)) & 0xffc0000000000000) |                                                    \
                        ((unsigned long)(__ppn_to_write) << 10) | ((unsigned long)(__perm) | (unsigned long)(__V));             \
}
#endif
