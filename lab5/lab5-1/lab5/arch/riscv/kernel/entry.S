.section .text.entry
.align 2
# .extern dummy
.globl _traps 
_traps:
# YOUR CODE HERE
# -----------
    # 1. save 32 registers and sepc, sstatus to stack

    csrr t2, sscratch
    beqz t2, notUser1
    csrrw sp, sscratch,sp
notUser1:
    addi sp, sp, -272
    csrr t1, sstatus
    sd t1, 264(sp)
    csrr t1, sepc
    sd t1, 256(sp)

    sd x31, 248(sp)
    sd x30, 240(sp)
    sd x29, 232(sp)
    sd x28, 224(sp)
    sd x27, 216(sp)
    sd x26, 208(sp)
    sd x25, 200(sp)
    sd x24, 192(sp)
    sd x23, 184(sp)
    sd x22, 176(sp)
    sd x21, 168(sp)
    sd x20, 160(sp)
    sd x19, 152(sp)
    sd x18, 144(sp)
    sd x17, 136(sp)
    sd x16, 128(sp)
    sd x15, 120(sp)
    sd x14, 112(sp)
    sd x13, 104(sp)
    sd x12, 96(sp)
    sd x11, 88(sp)
    sd x10, 80(sp)
    sd x9, 72(sp)
    sd x8, 64(sp)
    sd x7, 56(sp)
    sd x6, 48(sp)
    sd x5, 40(sp)
    sd x4, 32(sp)
    sd x3, 24(sp)
    sd x2, 16(sp)
    sd x1, 8(sp)      
    sd x0, 0(sp) // no use


# -----------

    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler
# -----------

    # 3. restore sepc, sstatus and 32 registers (x2(sp) should be restore last) from stack

    # ld t2, 264(sp) # problem!
    ld t1, 264(sp)
    csrw sstatus, t1
    ld t1, 256(sp)
    csrw sepc, t1
    ld x31, 248(sp)
    ld x30, 240(sp)
    ld x29, 232(sp)
    ld x28, 224(sp)
    ld x27, 216(sp)
    ld x26, 208(sp)
    ld x25, 200(sp)
    ld x24, 192(sp)
    ld x23, 184(sp)
    ld x22, 176(sp)
    ld x21, 168(sp)
    ld x20, 160(sp)
    ld x19, 152(sp)
    ld x18, 144(sp)
    ld x17, 136(sp)
    ld x16, 128(sp)
    ld x15, 120(sp)
    ld x14, 112(sp)
    ld x13, 104(sp)
    ld x12, 96(sp)
    ld x11, 88(sp)
    ld x10, 80(sp)
    ld x9, 72(sp)
    ld x8, 64(sp)
    ld x7, 56(sp)
    ld x6, 48(sp)
    ld x5, 40(sp)
    ld x4, 32(sp)
    ld x3, 24(sp)
    ld x2, 16(sp)
    ld x1, 8(sp)
    # ld x0, 0(sp)
    addi sp, sp, 272
    
    csrr t1, sscratch
    beqz t1, notUser2
    csrrw sp, sscratch,sp

notUser2:

    # -----------
        # 4. return from trap
        sret
    # -----------

# arch/riscv/kernel/entry.S


.global __dummy
__dummy:
   
    #la a0, dummy
    #csrw sepc, a0
    #csrr a0, sscratch   #U-mode sp

    #csrw sscratch, sp   #S-mode sp
    #addi sp,a0,0        #sp=U-mode sp

    csrrw sp, sscratch, sp
    la t0, 0 #set the return address uspp_start_address and return to 0
    csrw sepc, t0
    sret

.globl __switch_to
__switch_to:
    # save state to prev process
    sd ra,40(a0)#前面5个64位变量，这里的40是十进制
    sd sp,48(a0)
    sd s0,56(a0)
    sd s1,64(a0)
    sd s2,72(a0)
    sd s3,80(a0)
    sd s4,88(a0)
    sd s5,96(a0)
    sd s6,104(a0)
    sd s7,112(a0)
    sd s8,120(a0)
    sd s9,128(a0)
    sd s10,136(a0)
    sd s11,144(a0)
    csrr t0, sepc
    sd t0,152(a0)
    csrr t0, sstatus
    sd t0, 160(a0)
    csrr t0, sscratch
    sd t0, 168(a0)
    
    ld t0, 176(a1)      #pgn
    srli t0, t0, 12     #以4kb页面大小为单位
    li t1, (8 << 60)    #设置MODE 
    or t2, t0, t1
    csrw satp, t2
    sfence.vma zero, zero

    # restore state from next process
    ld ra,40(a1)
    ld sp,48(a1)
    ld s0,56(a1)
    ld s1,64(a1)
    ld s2,72(a1)
    ld s3,80(a1)
    ld s4,88(a1)
    ld s5,96(a1)
    ld s6,104(a1)
    ld s7,112(a1)
    ld s8,120(a1)
    ld s9,128(a1)
    ld s10,136(a1)
    ld s11,144(a1)
    ld t0, 152(a1)
    csrw sepc, t0
    ld t0, 160(a1)
    csrw sstatus, t0
    ld t0, 168(a1)
    csrw sscratch, t0
    ret
