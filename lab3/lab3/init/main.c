#include "printk.h"

extern void test();

int start_kernel() {
    printk("2021");
    printk(" Hello RISC-V\n");

    test(); // DO NOT DELETE !!!

	return 0;
}
