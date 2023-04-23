#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* syscall turns user space into kernel space. 
   Implement order:
    1. Check if a stack pointer is in user space.
    2. Copy syscall number from user stack.
    3. Copy arguments by syscall number, then do system call. */

/* Check if a stack pointer(or address) is in user space(or address): 0x8048000 ~ 0xc0000000. 
   If it is out of the space, then exit process.*/
void check_user_space(void *stack_ptr) {
  bool success;

}

/* Copy arguments of esp into arg as much as arg_cnt.
   i.e., copy arguments of system call into kernel. */
void copy_arguments(void *esp, int *arg, int arg_cnt) {

}

/* Given syscall number, implement call of service subroutines of paricular system call. */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /* Check whether or not user space. */
  check_user_space(f->esp);

  /* Copy arguments from user stack. */
  int arguments[32];
  copy_arguments(f->esp, arguments, 0);

  printf ("system call!\n");
  thread_exit ();
}
