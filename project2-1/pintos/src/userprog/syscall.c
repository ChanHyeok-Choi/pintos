#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

/* syscall turns user space into kernel space. 
   Implement order:
    1. Check if a stack pointer is in user space.
    2. Copy syscall number from user stack.
    3. Copy arguments by syscall number, then do system call. */

/* Terminates Pintos by calling shutdown_power_off() (declared in threads/init.h). 
   This should be seldom used, because you lose some information about possible deadlock 
   situations, etc. */
void halt (void) {
  shutdown_power_off();
}

/* Terminates the current user program, returning status to the kernel. 
   If the process's parent waits for it (see below), this is the status that will be returned.
   Conventionally, a status of 0 indicates success and nonzero values indicate errors.*/
void exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

/* Creates a new file called file initially initial_size bytes in size. 
   Returns true if successful, false otherwise. Creating a new file does not open it: 
   opening the new file is a separate operation which would require a open system call.*/
bool create (const char *file, unsigned initial_size) {
  if (file == NULL) {
    exit(-1);
  }
  lock_acquire (&filesys_lock);
  bool result = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return result;
}

/* Opens the file called file. Returns a nonnegative integer handle called a 
   "file descriptor" (fd), or -1 if the file could not be opened.*/
int open (const char *file) {
  if (file == NULL) {
    exit(-1);
  }
  lock_acquire (&filesys_lock);
  struct file *f = filesys_open(file);
  lock_release(&filesys_lock);
  if (f == NULL) {
    return -1;
  }
  // WE NEED FILE DESCRIPTOR!!!
  struct thread *cur = thread_current();
  int fd = cur->next_fd;
  cur->file_descriptor_table[fd] = f;
  cur->next_fd++;
  return fd;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes 
   all its open file descriptors, as if by calling this function for each one.*/
void close (int fd) {
  struct thread *cur = thread_current();
  struct file *f = cur->file_descriptor_table[fd];
  lock_acquire (&filesys_lock);
  file_close(f);
  lock_release(&filesys_lock);
  cur->file_descriptor_table[fd] = NULL;
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes 
   actually written, which may be less than size if some bytes could not be written.*/
int write (int fd, const void *buffer, unsigned size) {
  /* We should implenment only the following: 
     If fd == 1, writes to console: call putbuf(buffer, size) and return size. */
  struct thread *cur = thread_current();
  struct file *f = cur->file_descriptor_table[fd];
  
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  } else {
    /* Use lock to avoid concurrent access to file when read or write it. */
    lock_acquire(&filesys_lock);
    off_t file_size = file_write(f, buffer, size);
    lock_release(&filesys_lock);
    return (int) file_size;
  }
}

/* Check if a stack pointer(or address) is in user space(or address): 0x8048000 ~ 0xc0000000. 
   If it is out of the space, then exit process.*/
void check_user_space(void *stack_ptr) {
  // ASSERT(is_user_vaddr(stack_ptr));
  // if ((int)stack_ptr >= 0x8048000 || (unsigned int)stack_ptr <= 0xc0000000) {
  if(is_user_vaddr(stack_ptr)) {
    // pass
  } else {
    exit(-1);
  }
  if (pagedir_get_page(thread_current()->pagedir, stack_ptr) != NULL) {
    // pass
  } else {
    exit(-1);
  }
}

/* Copy arguments of esp into arg as much as arg_cnt.
   i.e., copy arguments of system call into kernel. */
void copy_arguments(void *esp, int *arg, int arg_cnt) {
  /* There are three cases for arguments: # of arguments are from 1 to 3. */

  // printf("aaaaaaa %x\n", esp+20);
  // hex_dump((uintptr_t)esp, esp, PHYS_BASE - esp, true);

  switch (arg_cnt) {
    case 1:
      check_user_space(esp+4);
      arg[0] = *(int *)(esp+4);
      break;
    case 2:
      check_user_space(esp+16);
      arg[0] = *(int *)(esp+16);
      arg[1] = *(int *)(esp+20);
      break;
    case 3:
      check_user_space(esp+20);
      arg[0] = *(int *)(esp+20);
      arg[1] = *(int *)(esp+24);
      arg[2] = *(int *)(esp+28);
      // printf("%d %x %d\n", arg[0], arg[1], arg[2]);
      break;
  };
}

/* Given syscall number, implement call of service subroutines of paricular system call. */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_number = *(int *)f->esp;
  /* Check whether or not user space. */
  check_user_space(f->esp);

  /* Copy arguments from user stack. */
  int args[3];
  
  switch(syscall_number) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      copy_arguments(f->esp, args, 1);
      exit(args[0]);
      break;
    case SYS_CREATE:
      copy_arguments(f->esp, args, 2);
      f->eax = create((const char *) args[0], (unsigned) args[1]);
      break;
    case SYS_OPEN:
      copy_arguments(f->esp, args, 1);
      f->eax = open((const char *) args[0]);
      break;
    case SYS_CLOSE:
      copy_arguments(f->esp, args, 1);
      close(args[0]);
      break;
    case SYS_WRITE:
      copy_arguments(f->esp, args, 3);
      f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
      break;
  }

  // printf ("system call!\n");
  // thread_exit ();
}
