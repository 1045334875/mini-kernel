.extern start_kernel
.extern kalloc
.extern mm_init
.extern task_init
.extern clock_set_next_event
.extern early_pgtbl
.extern setup_vm
.extern setup_vm_final

    .section .text.init
    .globl _start
    
_start:
    # YOUR CODE HERE
    la sp,boot_stack_top
         #lab6
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final

    call task_init
    ## set stvec = _traps
    la a0 ,_traps
    csrw stvec, a0

    ## set sie[STIE] = 1
    #csrci sie, 0x10  #stip is located in fifth bit in sie
    csrr a0,sie
    ori a0,a0,32
    csrw sie,a0
        
    ## set sstatus[SIE] = 1
    #csrci sstatus, 0x01 #sie is located in second bit in sstatus
    #csrr a0,sstatus
    #ori a0,a0,2
    #csrw sstatus,a0
   
    ## set first time interrupt
    call clock_set_next_event
    call start_kernel

#sys3 lab3 new    
relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t1, 0xFFFFFFDF80000000
    add ra, ra, t1
    add sp, sp, t1
    # set satp with early_pgtbl
    
    la t3,  early_pgtbl;
    srli t3, t3, 12; # PPN = PA >>  12
    li t1,  0x8000000000000000
    or t3, t1, t3
    csrw satp, t3
    
    # flush tlb
    sfence.vma zero, zero

    ret


    .section .bss.stack  
    .globl boot_stack
boot_stack:         
    .space 0x1000 # <-- the stack size is 4kb

    .globl boot_stack_top
boot_stack_top:
