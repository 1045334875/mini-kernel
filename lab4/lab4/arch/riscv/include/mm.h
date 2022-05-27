

#ifndef _MM_H_
#define _MM_H_
#include "types.h"
struct run {
    struct run *next;
};

void mm_init();

uint64 kalloc();
void kfree(uint64);
//create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);

#endif
