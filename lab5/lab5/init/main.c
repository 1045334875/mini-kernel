#include "printk.h"

extern void test();

int start_kernel() {
    //printk("2021");
    printk("[S-MODE] Hello RISC-V\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
