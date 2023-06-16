#include "frame.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

struct list LRU_list;           /* list of physical pages allocated to user process. */
struct lock LRU_lock;           /* Lock for LRU. */
struct list_elem *clock_for_LRU; /* Element for clock algorithm. */

static struct list_elem* get_next_LRU_clock(void);

/* Initialize LRU list. */
void init_LRU_list (void) {
    list_init(&LRU_list);
    lock_init(&LRU_lock);
    clock_for_LRU = NULL;
}

/* Insert user page into end of LRU list. */
void push_back_LRU_list (struct page* page) {
    list_push_back (&LRU_list, &page->LRU);
}

/* Remove user page from LRU list. */
void pop_LRU_list (struct page* page) {
    /* If element to remove is equal to clocking, update it to next element. */
    if (&page->LRU == clock_for_LRU) {
        clock_for_LRU = list_next(clock_for_LRU);
    }
    list_remove (&page->LRU);
}

/* *** Need to change allocation function from palloc_get_page() to page_alloc(). *** */
/* Whenever user page allocation or deletion, insert or remove relevant page to or from LRU list. */
struct page* page_alloc (enum palloc_flags flags) {
    lock_acquire(&LRU_lock);
    uint8_t *kpage;
    /* Allocate page. */
    kpage = palloc_get_page(flags);
    /* When palloc_get_page() cannot allocate memory, call select_victim_out(). */
    while (kpage == NULL) {
        select_victim_out();
        kpage = palloc_get_page(flags);
    }

    /* Allocate and initialize page. */
    struct page *page = (struct page *) malloc(sizeof(struct page));
    page->kaddr = kpage;
    page->thread = thread_current();
    
    /* Insert page into LRU list by push_back_LRU_list(). */
    push_back_LRU_list(page);

    lock_release(&LRU_lock);
    
    return page;
}

void free_page (void *_kaddr) {
    lock_acquire(&LRU_lock);
    /* Search page w.r.t. kaddr in LRU list. */
    struct list_elem *e;
    for (e = list_begin(&LRU_list); e != list_end(&LRU_list); e = list_next(e)) {
        struct page *current_page = list_entry(e, struct page, LRU);
        /* If matching found, call free_LRU_page. */
        if (current_page->kaddr == _kaddr) {
            free_LRU_page(current_page);
            break;
        }
    }

    lock_release(&LRU_lock);
}

void free_LRU_page (struct page* page) {
    /* Remove page from LRU. */
    pop_LRU_list(page);
    /* Free memory space allocated to page. */
    // pagedir_clear_page(page->thread->pagedir, page->vmE->vaddr);
    pagedir_clear_page(page->thread->pagedir, pg_round_down(page->vmE->vaddr));
    palloc_free_page(page->kaddr);
    free(page);
}

static struct list_elem* get_next_LRU_clock (void) {
    /* To do task moving next through LRU list by clock algorithm. */
    /* Return a location of next of LRU list. If current LRU list is a last node, return NULL. */
    // if (list_empty(&LRU_list) || clock_for_LRU == list_end(&LRU_list))
    //         return NULL;
    // return list_next(clock_for_LRU);

    if (list_empty(&LRU_list))
        return NULL;

    if (clock_for_LRU == NULL || clock_for_LRU == list_end(&LRU_list))
        return list_begin(&LRU_list);
    
    if (list_end(&LRU_list) == list_next(clock_for_LRU))
        return list_begin(&LRU_list);
    else
        return list_next(clock_for_LRU);

    return clock_for_LRU;
}

/* When there is not enough physical memory, retain spare memory by clock algorithm.
   Select victim page by clock algorithm, and release it out, then return the virtual address
   of spare page. */
void select_victim_out (void) {
    /* Search. */
    clock_for_LRU = get_next_LRU_clock();
    struct page *page = list_entry(clock_for_LRU, struct page, LRU);
    while (pagedir_is_accessed(page->thread->pagedir, page->vmE->vaddr)) {
        pagedir_set_accessed(page->thread->pagedir, page->vmE->vaddr, false);
        clock_for_LRU = get_next_LRU_clock();
        page = list_entry(clock_for_LRU, struct page, LRU);
    }

    /* Release phsycal memory will be different according to vm_entry's type. */
    switch (page->vmE->type) {
        case VM_ELF: /* If dirty bit is 1, free page after writing it into swap partition by swap_in(). And for demanding paging, change the type to VM_SWAP. */
            if (pagedir_is_dirty(page->thread->pagedir, page->vmE->vaddr)) {
                page->vmE->swap_slot = swap_out(page->kaddr);
                page->vmE->type = VM_SWAP;
            }
            break;
        case VM_FILE: /* If dirty bit is 1, free page after writing it into file. Otherwise, directly free it. */
            if (pagedir_is_dirty(page->thread->pagedir, page->vmE->vaddr)) {
                file_write_at(page->vmE->file, page->vmE->vaddr, page->vmE->read_bytes, page->vmE->offset);
            }
            break;
        case VM_SWAP: /* Always write it into swap partition. */
            page->vmE->swap_slot = swap_out(page->kaddr);
            break;
    }

    page->vmE->load_flag = false;
    free_LRU_page(page);
}