#ifndef _FRAME_H_
#define _FRAME_H_

#include "list.h"
#include "threads/palloc.h"
#include "vm/page.h"

void init_LRU_list (void);

void push_back_LRU_list (struct page* page);
void pop_LRU_list (struct page* page);

struct page* page_alloc (enum palloc_flags flags);
void free_page (void *_kaddr);
void free_LRU_page (struct page* page);

void select_victim_out (void);

#endif