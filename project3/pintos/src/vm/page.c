#include "page.h"
#include "lib/string.h"
#include "lib/stdbool.h"
#include "lib/kernel/hash.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"

static unsigned hash_func_for_vm (const struct hash_elem *e, void *aux UNUSED);
static bool less_func_for_vm (const struct hash_elem *h1, const struct hash_elem *h2);

/* Initialize hash table by using hash_init(). */
void init_hash_for_vm (struct hash *vm) {
    hash_init(vm, hash_func_for_vm, less_func_for_vm, NULL);
}

/* After searching struct vm_entry for element by hash_entry(), return hash value
   for vaddr of vm_entry by using hash_int(). */
static unsigned hash_func_for_vm (const struct hash_elem *e, void *aux UNUSED) {
    struct vm_entry* vmE = hash_entry(e, struct vm_entry, hash_elem);
    unsigned hash_value = hash_int((int) vmE->vaddr);
    return hash_value;
}

/* Compare each vaddr after getting struct vm_entry for each element by hash_entry(). */
static bool less_func_for_vm (const struct hash_elem *h1, const struct hash_elem *h2) {
    struct vm_entry* vmE1 = hash_entry(h1, struct vm_entry, hash_elem);
    struct vm_entry* vmE2 = hash_entry(h2, struct vm_entry, hash_elem);
    bool result = vmE1->vaddr < vmE2->vaddr;
    return result;
}

/* Insert vm_entry into hash table by using hash_insert(). If insertion is succeeded, then 
   return ture, otherwise false. */
bool insert_vm_entry (struct hash *vm, struct vm_entry *vmE) {
    struct hash_elem* old = hash_insert(vm, &vmE->hash_elem);
    bool success;
    if (old == NULL) {
        success = true;
    } else {
        success = false;
    }
    return success;
}

/* Remove vm_entry from hash table by using hash_delete(). */
bool delete_vm_entry (struct hash *vm, struct vm_entry *vmE) {
    struct hash_elem* old = hash_delete(vm, &vmE->hash_elem);
    bool success;
    if (old == NULL) {
        success = false;
    } else {
        success = true;
    } 
    return success;
}

/* Return vm_entry w.r.t. vaddr by using hash_find(). If no exists, return NULL. */
struct vm_entry *find_vm_entry (void *vaddr) {
    struct vm_entry* vmE = NULL;
    vmE->vaddr = pg_round_down(vaddr);
    struct hash_elem* e = hash_find(&thread_current()->vm, &vmE->hash_elem);
    if (e != NULL)
        return hash_entry(e, struct vm_entry, hash_elem);
    return NULL;
}

/* Remove bucket list and vm_entries of hash table by using hash_destory(). */
void destroy_vm_entries (struct hash *vm) {
    hash_destroy(vm, destroy_vm_func);
}

/* Performs some operation on hash element E, given auxiliary data AUX.*/
void destroy_vm_func (struct hash_elem *e, void *aux UNUSED) {
    struct vm_entry *vmE = hash_entry(e, struct vm_entry, hash_elem);
    /* If vm_entry of a loaded page, free page and page mapping by palloc_free_page() and pagedir_clear_page(). */
    if (vmE->load_flag == true) {
        void* page = pagedir_get_page(thread_current()->pagedir, vmE->vaddr);
        palloc_free_page(page);
        pagedir_clear_page(thread_current()->pagedir, vmE->vaddr);
    }
    free(vmE);
}

/* Load from page of disk to physical memory*/
bool load_disk_page(void* kaddr, struct vm_entry *vmE) {
    /* Read file, then if success, return true.
       read vm_entry file to physical memory. 
       If fail to write all of 4KB, fill the rest with 0. */
    off_t disk_file = file_read_at(vmE->file, kaddr, vmE->read_bytes, vmE->offset);
    if ((int) vmE->read_bytes == disk_file) {
        memset(kaddr + vmE->read_bytes, 0, vmE->zero_bytes);
        return true;
    }
    return false;
}