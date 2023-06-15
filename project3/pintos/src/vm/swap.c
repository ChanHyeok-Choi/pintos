#include "swap.h"
#include "bitmap.h"
#include "devices/block.h"
#include "threads/vaddr.h"

/* Exists in memory (global). Notice whether swap slot is available or not. */
struct bitmap *bitmap_for_swap;

/* Initialize swap space. */
void init_swap (void) {
    bitmap_for_swap = bitmap_create(4096);
}

/* Record page w.r.t. kaddr into swap partition. Then, return swap slot index recording page. */
size_t swap_out (void *kaddr) {
    /* Search spare swap slot by first-fit algorithm to bitmap. */
    size_t idx_for_swap = bitmap_scan(bitmap_for_swap, 0, 1, false);
    /* If there is an enough swap space in first, write the page into disk. */
    if (idx_for_swap != BITMAP_ERROR) {
        struct block *disk_for_swap = block_get_role(BLOCK_SWAP);
        int i;
        for (i = 0; i ; i++) {
            block_write(disk_for_swap, (PGSIZE/BLOCK_SECTOR_SIZE)*idx_for_swap + i, BLOCK_SECTOR_SIZE*i + kaddr);
        }
        bitmap_reset(bitmap_for_swap, idx_for_swap);
    }
    return idx_for_swap;
}

/* Copy swap slot data w.r.t. idx into kaddr. */
void swap_in (size_t idx_for_swap, void *kaddr) {
    /* If there is already the used swap space w.r.t. idx_for_swap, then load the blocks into physical pages. */
    if (bitmap_test(bitmap_for_swap, idx_for_swap)) {
        struct block *disk_for_swap = block_get_role(BLOCK_SWAP);
        int i;
        for (i = 0; i ; i++) {
            block_read(disk_for_swap, (PGSIZE/BLOCK_SECTOR_SIZE)*idx_for_swap + i, BLOCK_SECTOR_SIZE*i + kaddr);
        }
        bitmap_reset(bitmap_for_swap, idx_for_swap);
    }
}