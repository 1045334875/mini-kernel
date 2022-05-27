.section .text.entry
.align 2
# .extern dummy
.globl _traps 
_traps:
     csrr t0,sscratch
     beqz t0,notUser1
     csrrw sp,sscratch,sp
notUser1:
        # 1. save 32 registers and sepc to stack
     sd zero,-8(sp)
     sd ra, -16(sp)
     sd sp, -24(sp)
     sd gp, -32(sp)
     sd tp, -40(sp)
     sd t0, -48(sp)
     sd t1, -56(sp)
     sd t2, -64(sp)
     sd s0, -72(sp)
     sd s1, -80(sp)
     sd a0, -88(sp)
     sd a1, -96(sp)
     sd a2, -104(sp)
     sd a3, -112(sp)
     sd a4, -120(sp)
     sd a5, -128(sp)
     sd a6, -136(sp)
     sd a7, -144(sp)
     sd s2, -152(sp)
     sd s3, -160(sp)
     sd s4, -168(sp)
     sd s5, -176(sp)
     sd s6, -184(sp)
     sd s7, -192(sp)
     sd s8, -200(sp)
     sd s9, -208(sp)
     sd s10, -216(sp)
     sd s11, -224(sp)
     sd t3, -232(sp)
     sd t4, -240(sp)
     sd t5, -248(sp)
     sd t6, -256(sp)
     csrr t0,sepc
     sd t0,-264(sp)
     csrr t0,sstatus
     sd t0,-272(sp)
     addi sp,sp,-272


# -----------

    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler
# -----------

    # 3. restore sepc, sstatus and 32 registers (x2(sp) should be restore last) from stack

     ld t0,0(sp)
     csrw sstatus,t0
     ld t0,8(sp)
     csrw sepc,t0
     ld t6,16(sp)
     ld t5,24(sp)
     ld t4,32(sp)
     ld t3,40(sp)
     ld s11,48(sp)
     ld s10,56(sp)
     ld s9,64(sp)
     ld s8,72(sp)
     ld s7,80(sp)
     ld s6,88(sp)
     ld s5,96(sp)
     ld s4,104(sp)
     ld s3,112(sp)
     ld s2,120(sp)
     ld a7,128(sp)
     ld a6,136(sp)
     ld a5,144(sp)
     ld a4,152(sp)
     ld a3,160(sp)
     ld a2,168(sp)
     ld a1,176(sp)
     ld a0,184(sp)
     ld s1,192(sp)
     ld s0,200(sp)
     ld t2,208(sp)
     ld t1,216(sp)
     ld t0,224(sp)
     ld tp,232(sp)
     ld gp,240(sp)
     ld ra,256(sp)
     ld sp,248(sp)

     csrr t0,sscratch
     beqz t0,notUser2
     csrrw sp,sscratch,sp

    
notUser2:
        # 4. return from trap
     sret

# arch/riscv/kernel/entry.S

.global ret_from_fork
ret_from_fork:
    ld t0, 0(a0)
    csrw sstatus, t0
    ld t0, 8(a0)
    addi t0,t0,4 # sepc = ecall+4
    csrw sepc, t0
    ld x31, 16(a0)
    ld x30, 24(a0)
    ld x29, 32(a0)
    ld x28, 40(a0)
    ld x27, 48(a0)
    ld x26, 56(a0)
    ld x25, 64(a0)
    ld x24, 72(a0)
    ld x23, 80(a0)
    ld x22, 88(a0)
    ld x21, 96(a0)
    ld x20, 104(a0)
    ld x19, 112(a0)
    ld x18, 120(a0)
    ld x17, 128(a0)
    ld x16, 136(a0)
    ld x15, 144(a0)
    ld x14, 152(a0)
    ld x13, 160(a0)
    ld x12, 168(a0)
    ld x11, 176(a0)
    ld x9, 192(a0)
    ld x8, 200(a0)
    ld x7, 208(a0)
    ld x6, 216(a0)
    ld x5, 224(a0)
    ld x4, 232(a0)
    ld x3, 240(a0)
    ld x2, 248(a0)
    ld x1, 256(a0)
    ld x10, 184(a0)
    sret


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
