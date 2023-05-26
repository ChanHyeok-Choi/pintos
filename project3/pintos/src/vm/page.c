#include "page.h"
#include "lib/kernel/hash.h"

/* Initialize hash table by using hash_init(). */
void init_hash_for_vm (struct hash *vm) {
    hash_init(vm, &hash_func_for_vm, &less_func_for_vm, NULL);
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