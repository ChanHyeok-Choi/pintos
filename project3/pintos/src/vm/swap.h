#ifndef _SWAP_H_
#define _SWAP_H_

#include <stddef.h>

void init_swap (void);
size_t swap_out (void *kaddr);
void swap_in (size_t idx_for_swap, void *kaddr);

#endif