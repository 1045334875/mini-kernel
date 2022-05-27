#include "defs.h"
#include "string.h"
#include "mm.h"
#include "printk.h"
#include "sbi.h"
#include "proc.h"


/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));//地址按0x1000对齐
//这里只实现了第一级页表，就是第一个512的模块，要读懂手册才能做。
void setup_vm(void) {
    early_pgtbl[2]=(unsigned long)(0X2<<28) | 0xf;//XWRV=1111
    early_pgtbl[384]=(unsigned long)(0x2<<28) | 0xf;//<<28 -> VPN[2] superpage
    //printk("enter setup vm\n");
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    	为什么是1G
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));


/* 创建多级页表映射关系 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
   // printk("enter create mapping\n");
   unsigned long va_temp, pa_temp;
    for(va_temp=va,pa_temp=pa; va_temp < va+sz; va_temp += PGSIZE, pa_temp += PGSIZE){
        unsigned long *cur_page_addr, *cur_pte_addr, cur_pte_data;
        cur_page_addr=pgtbl;
        for(int level = 2; level > 0 ; level--){
            if(level==2){
            //VPN[2]->PPN[1]
            cur_pte_data = cur_page_addr[(va_temp>>30) & (PGNUM-1)];//va[39:31]=VPN[2]
            cur_pte_addr = &cur_page_addr[(va_temp>>30) & (PGNUM-1)];
            }
            else if(level==1){
            //VPN[1]->PPN[0]
            cur_pte_data = cur_page_addr[(va_temp>>21) & (PGNUM-1)];//va[30:22]=VPN[1]
            cur_pte_addr = &cur_page_addr[(va_temp>>21) & (PGNUM-1)];
            }
            if((cur_pte_data&1)){//valid, then find next page table entry
                cur_page_addr = (unsigned long *)(((cur_pte_data>>10)<<12) + PA2VA_OFFSET);//cur_page_data[54:11]=PPN2
            }
            else {//not valid
                cur_page_addr = (uint64 *)kalloc();
                //printk("new malloc:&%lx\n",cur_page_addr);
                memset(cur_page_addr, 0, PGSIZE);
                WritePTE(cur_pte_addr, ((unsigned long)cur_page_addr - PA2VA_OFFSET)>>12, 0, 1);
                //printk("&%lx = %lx\n",cur_pte_addr, *cur_pte_addr);
                //malloc new page and set entry
            }
        }
        //VPN[0]
        WritePTE(&cur_page_addr[(va_temp>>12) & (PGNUM-1)], (pa_temp)>>12, perm, 1);
        //printk("&%lx = %lx\n",&cur_page_addr[(va_temp>>12) & (PGNUM-1)], cur_page_addr[(va_temp>>12) & (PGNUM-1)]);
        //unsigned long high10 = ((unsigned long)*cur_page_addr) & 0xffc0000000000000;
        //cur_page_addr[(va_temp>>12) & (PGNUM-1)] = (unsigned long)((unsigned long)pa_temp >> 12)<<10 | perm | high10 | 1;
        //printk("5\n");
    }
}

extern char _skernel[];
extern char _stext[];
extern char _etext[];
extern char _sdata[];
extern char _srodata[];
extern char _erodata[];
extern char _ekernel[];

void setup_vm_final(void) {
    //printk("enter vm final\n");
    memset(swapper_pg_dir, 0x0, PGSIZE);


    // No OpenSBI mapping required
    // mapping kernel text X|-|R|V
    
    unsigned long va = VM_START + OPENSBI_SIZE;
    
    unsigned long pa = PHY_START + OPENSBI_SIZE;
    
    unsigned long text_length = (unsigned long)_srodata - (unsigned long)_stext;
    
    create_mapping(swapper_pg_dir, va, pa, text_length, 11);//1011
    //printk("text memory: %lx ~ %lx\n", va, va+text_length);

    // mapping kernel rodata -|-|R|V
    va += text_length;
    pa += text_length;
    unsigned long rodata_length = (unsigned long)_sdata - (unsigned long)_srodata;
    create_mapping(swapper_pg_dir, va, pa, rodata_length, 3);//0011
    //printk("rodata memory: %lx ~ %lx\n", va, va+rodata_length);
    
    // mapping other memory -|W|R|V
    va += rodata_length;
    pa += rodata_length;
    unsigned long other_length = PHY_SIZE - rodata_length - text_length -OPENSBI_SIZE;//(unsigned long)_ekernel - (unsigned long)_sdata;
    create_mapping(swapper_pg_dir, va, pa, other_length, 7);//0111
    //printk("other memory: %lx ~ %lx\n", va, va+other_length);
    
    // set satp with swapper_pg_dir
    unsigned long temp = (unsigned long)swapper_pg_dir - PA2VA_OFFSET;
    temp = (unsigned long)temp >> 12;
    temp = (0x000fffffffffff & temp) | 0x8000000000000000;
    csr_write(satp, temp);
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    return;
}



