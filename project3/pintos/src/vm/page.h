#ifndef VM_PAGE_H
#define VM_PAGE_H
/* We need virtual memory entry to reference page table. 
   Example:
    1. Suppose that we access virtual address of 0x0804bec0 w.r.t. 0x0804b000 (0x is page directory index, 0804b is virtual page number index, and ec0 is offset).
    2. Then, 0x0804b000 page will be requested, where a frame is not mapped in the reference page table.
    3. So, allocate physical memory for that frame.
    4. Load a related file contents into physical memory. 
    5. Map virtual page with physical frame, in that we should check read/write authorization. 
       (Because reading an information for offset is needed when allocation of physical memory.)
   
   Hash table for managing vm_entries:
      On process created:
         initialize hash table
         create vm_entries for virtual page of process
      On process executing:
         When page fault occurs, search vm_entry w.r.t. address where page fault occurs through hash table.
      On process terminated:
         Free bucket list of hash table and data structure for vm_entry.
    */

#include "lib/stdint.h"
#include "lib/stddef.h"
#include "lib/kernel/hash.h"
#include "lib/debug.h"
#include "lib/stdbool.h"

#define VM_ELF 0
#define VM_FILE 1
#define VM_SWAP 2

struct mm_file {
   int mmId;               /* Once succeeded, returned mapping id. */
   struct file* file;      /* File object of mapping file. */
   struct list_elem elem;  /* Struct for connecting list of mm_files. */
   struct list vmE_list;   /* List of all of vm_entry w.r.t. mm_file. */
};

/* Data structure that loads necessary pages by separating logical and physical addresses. */
struct vm_entry {
   uint8_t type;              /* Type of VM_ELF, VM_FILE, and VM_SWAP. */
   void *vaddr;               /* Virtual page number of vm_entry. */
   bool writable_flag;        /* Flag that determines whether write is possible or not. Ture: writable, False: impossbile. */
   
   bool load_flag;            /* Flag that informs whether load on physical memory is done or not. */
   struct file* file;         /* File mapped to virtual address. */

   size_t offset;             /* Offset of file to read. */
   size_t read_bytes;         /* Size of data that is written to virtual page. */
   size_t zero_bytes;         /* Bytes of remained page to fill with 0. */

   struct hash_elem hash_elem;/* Hash table element. */

   struct list_elem mm_elem;  /* mm list element. */
   size_t swap_slot;          /* Swap slot. */
};

void init_hash_for_vm (struct hash *vm);

bool insert_vm_entry (struct hash *vm, struct vm_entry *vmE);
bool delete_vm_entry (struct hash *vm, struct vm_entry *vmE);

struct vm_entry *find_vm_entry (void *vaddr);

void destroy_vm_entries (struct hash *vm);
void destroy_vm_func (struct hash_elem *e, void *aux UNUSED);

bool load_disk_page(void* kaddr, struct vm_entry *vmE);

#endif