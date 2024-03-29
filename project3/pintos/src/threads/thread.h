#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "filesys/file.h"
#include "synch.h"
#include "lib/kernel/hash.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* File descriptor table size. */
#define FDT_MAX_SIZE 128

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
    int64_t sleep_ticks;                /* Remained ticks for sleeping. */

    /* Shared between thread.c and userprog/syscall.c. */
    struct file **file_descriptor_table;/* File descriptor table in kernel address space. */
    int next_fd;                        /* Next file descripter. */

    /* Shared between thread.c and userprog/process.c & syscall.c. */
    struct thread *parent;              /* Process descriptor pointer of parent process. */
    bool load_status;                   /* Load status of process. Normal load: True, else: False. */
    bool exit_flag;                     /* Exit flag to check whether a thread terminates or not. */
    int exit_status;                    /* Exit status of process. Normal exit: 0, else: others. */
    struct semaphore wait_sema;         /* Semaphore for wait. */
    struct semaphore load_sema;         /* Semaphore for load. */
    struct list child_list;             /* List for child process. */
    struct list_elem child_elem;        /* Child process element. */

    struct file* executing_file;        /* Current executing file. */

    /* Shared between thread.c and vm/page.c. */
    /* Since virtual address space is allocated each process, hash table for managing virtual pages should be defined. */
    struct hash vm;                     /* Hash table managing virtual address space that thread has. */

    struct list mm_list;                /* List for files mapping memory. */
    int next_mmId;                      /* Pointer to next mm. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

struct thread *get_child_thread_by_tid (tid_t child_tid);
void remove_child_thread (struct thread *ct);

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

/* Add a function that compares priority between threads. */
bool compare_priority (const struct list_elem *a, 
                       const struct list_elem *b, 
                       void *aux UNUSED);
/* Add a function that compares priority between current_thread and threads in ready_list. */
void currrent_list_compare_priority(void);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Add a function that compares sleep_ticks between threads instead of list_less_func. */
bool less_than_sleep_tick (const struct list_elem *a, 
                           const struct list_elem *b, 
                           void *aux UNUSED);
/* Add a function that makes a thread to be a sleep state. */
void thread_sleep (int64_t ticks);
/* Add a function that iterates sleep_list and wake up a thread 
   with least sleep_ticks.*/
void thread_wake_up (int64_t ticks);
/* Add a function that updates a global variable 'least_sleep_tick'. */
// void update_least_sleep_tick (void);
/* Add a function that returns least_sleep_tick. */
// int64_t get_least_sleep_tick (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

#endif /* threads/thread.h */
