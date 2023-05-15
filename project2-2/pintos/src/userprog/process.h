#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/file.h"

/* Separate necessary functions doing task w.r.t. file_descriptor. */
int process_add_file(struct file* f);
struct file* process_get_file(int fd);
void process_close_file(int fd);
tid_t process_execute (const char *file_name);
void save_user_stack(char ** tokens, int arg_cnt, void **esp);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
