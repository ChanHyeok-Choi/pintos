#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "vm/page.h"

struct lock filesys_lock;

void syscall_init (void);
void halt (void);
void exit (int status);
bool create (const char *file, unsigned initial_size);
int open (const char *file);
void close (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
int wait (tid_t tid);
tid_t exec(const char *cmd_line);
bool remove (const char *file);
int filesize (int fd);
void seek (int fd, unsigned position);
unsigned tell (int fd);
// void check_user_space(void *stack_ptr);
struct vm_entry *check_user_space (void *addr, void *stack_ptr UNUSED);
void check_valid_buffer (void *buffer, unsigned size, void *stack_ptr, bool writable);
void check_valid_string (const void* str, void* stack_ptr);
void copy_arguments(void *esp, int *arg, int arg_cnt);

int mmap (int fd, void *addr);
void munmap (int mmId);
void do_munmap (struct mm_file* _mm_file);

#endif /* userprog/syscall.h */
