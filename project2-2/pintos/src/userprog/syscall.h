#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/synch.h"

struct lock filesys_lock;

void syscall_init (void);
void halt (void);
void exit (int status);
bool create (const char *file, unsigned initial_size);
int open (const char *file);
void close (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void check_user_space(void *stack_ptr);
void copy_arguments(void *esp, int *arg, int arg_cnt);

#endif /* userprog/syscall.h */
