#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "vm/page.h"

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
  thread_current()->exit_status = status;
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
  bool result = filesys_create(file, initial_size);
  return result;
}

/* Opens the file called file. Returns a nonnegative integer handle called a 
   "file descriptor" (fd), or -1 if the file could not be opened.*/
int open (const char *file) {
  if (file == NULL) {
    exit(-1);
  }
  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  if (f == NULL) {
    lock_release(&filesys_lock);
    return -1;
  }
  // WE NEED FILE DESCRIPTOR!!!
  int fd = add_file_descriptor(f);
  lock_release(&filesys_lock);
  return fd;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes 
   all its open file descriptors, as if by calling this function for each one.*/
void close (int fd) {
  close_file_descriptor(fd);
  // thread_current()->next_fd--;
}

/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes 
   actually read (0 at end of file), or -1 if the file could not be read (due to a 
   condition other than end of file). Fd 0 reads from the keyboard using input_getc().*/
int read (int fd, void *buffer, unsigned size) {
  struct file *f;
  int file_size;
  
  if (fd == 0) {
    /* Read input from keyboard. */
    char *buffer_ = buffer;
    unsigned i;
    for (i = 0; i < size; i++) {
      buffer_[i] = input_getc();
      if (buffer_[i] == '\0') {
        break;
      }
    }
    return i;
  } else {
    f = get_file_descriptor(fd);
    if (f == NULL) {
      exit(-1); 
    }
    lock_acquire(&filesys_lock);
    file_size = file_read(f, buffer, size);
    lock_release(&filesys_lock);
  }
  return file_size;
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes 
   actually written, which may be less than size if some bytes could not be written.*/
int write (int fd, const void *buffer, unsigned size) {
  /* We should implenment only the following: 
     If fd == 1, writes to console: call putbuf(buffer, size) and return size. */
  struct file *f;
  
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  } else {
    f = get_file_descriptor(fd);
    /* Use lock to avoid concurrent access to file when read or write it. */
    if (f == NULL) {
      return -1;
    }
    lock_acquire(&filesys_lock);
    off_t file_size = file_write(f, buffer, size);
    lock_release(&filesys_lock);
    return (int) file_size;
  }
}

/* Waits for a child process pid and retrieves the child's exit status.
   If pid is still alive, waits until it terminates. Then, returns the status that pid passed to exit. 
   If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an exception), 
   wait(pid) must return -1. It is perfectly legal for a parent process to wait for child processes 
   that have already terminated by the time the parent calls wait, but the kernel must still allow 
   the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel.*/
int wait (tid_t tid) {
  return process_wait(tid);
}

/* Executes a new process by parsing the command line string.
   The new process is loaded and scheduled to run.
   Returns the process ID (tid) of the new process, or -1 if the execution fails. */
tid_t exec(const char *cmd_line) {
  tid_t tid = process_execute(cmd_line);
  /* If the child process was created successfully, wait for the child process to be loaded into memory */
  // if (tid == -1) {
    // struct thread *child = get_child_thread_by_tid(tid);
    // struct thread *parent = thread_current();
    // sema_down(&parent->load_sema);
    
    // if (!parent->load_status) {
    //   return process_wait(tid);
    // }
  // }
  return tid;
}

/* Deletes the file called file. Returns true if successful, false otherwise.
   A file may be removed regardless of whether it is open or closed, and removing 
   an open file does not close it. See Removing an Open File, for details. */
bool remove (const char *file) {
  if (file == NULL) {
    exit(-1);
  }
  return filesys_remove(file);
}

/* Returns the size, in bytes, of the file open as fd. */
int filesize (int fd) {
  struct file *f = get_file_descriptor(fd);
  if (f == NULL) {
    exit(-1);
  }
  return file_length(f);
}

/* Changes the next byte to be read or written in open file fd to position, 
   expressed in bytes from the beginning of the file. (Thus, a position of 
   0 is the file's start.)*/
void seek (int fd, unsigned position) {
  struct file *f = get_file_descriptor(fd);
  if (f == NULL) {
    exit(-1);
  }
  file_seek(f, position);
}

/* Returns the position of the next byte to be read or written in open file fd,
   expressed in bytes from the beginning of the file. */
unsigned tell (int fd) {
  struct file *f = get_file_descriptor(fd);
  if (f == NULL) {
    exit(-1);
  }
  return file_tell(f);
}

/* Check if a stack pointer(or address) is in user space(or address): 0x8048000 ~ 0xc0000000. 
   If it is out of the space, then exit process.*/
// void check_user_space(void *stack_ptr) {
//   // ASSERT(is_user_vaddr(stack_ptr));
//   // if ((int)stack_ptr >= 0x8048000 || (unsigned int)stack_ptr <= 0xc0000000) {
//   if(is_user_vaddr(stack_ptr)) {
//     // pass
//   } else {
//     exit(-1);
//   }
//   if (pagedir_get_page(thread_current()->pagedir, stack_ptr) != NULL) {
//     // pass
//   } else {
//     exit(-1);
//   }
// }

struct vm_entry* check_user_space (void *addr, void *stack_ptr UNUSED) {
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
  /* If addr exists in vm_entry, return vm_entry by using find_vm_entry(). */
  return find_vm_entry(addr);
}

/* Check whether buffer address is valid virtual address or not. It is applied to read() & write() system call. */
void check_valid_buffer (void *buffer, unsigned size, void *stack_ptr, bool writable) {
  struct vm_entry *vmE;
  char *buffer_i = (char *) buffer;
  unsigned i;
  for (i = 0; i < size; i++) {
    vmE = check_user_space((void *) buffer_i, stack_ptr);
    if (vmE != NULL) {
      if (writable) {
        if (!vmE->writable_flag) {
          exit(-1);
        }
      }
    }
    buffer_i++;
  }
}

/* Check whether string address in system call is valid or not. This is applied to exec() & open() system call. */
void check_valid_string (const void* str, void* stack_ptr) {
  char *str_i = (char *) str;
  struct vm_entry *vmE;
  while (*str_i != 0) {
    str_i++;
    vmE = check_user_space((void *) str_i, stack_ptr);
    if (vmE == NULL)
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
      check_user_space(esp+4, esp+4);
      arg[0] = *(int *)(esp+4);
      break;
    case 2:
      check_user_space(esp+16, esp+16);
      arg[0] = *(int *)(esp+16);
      arg[1] = *(int *)(esp+20);
      break;
    case 3:
      check_user_space(esp+20, esp+20);
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
  check_user_space(f->esp, f->esp);

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
      check_valid_string((const char *) args[0], f->esp);
      f->eax = open((const char *) args[0]);
      break;
    case SYS_CLOSE:
      copy_arguments(f->esp, args, 1);
      close(args[0]);
      break;
    case SYS_READ:
      copy_arguments(f->esp, args, 3);
      check_valid_buffer((void *) args[1], (unsigned) args[2], f->esp, true);
      f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
      break;
    case SYS_WRITE:
      copy_arguments(f->esp, args, 3);
      // check_valid_buffer((void *) args[1], (unsigned) args[2], f->esp, true);
      check_valid_string((const void*) args[1], f->esp);
      f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
      break;
    case SYS_WAIT:
      copy_arguments(f->esp, args, 1);
      f->eax = wait((tid_t) args[0]);
      break;
    case SYS_EXEC:
      copy_arguments(f->esp, args, 1);
      check_valid_string((const char *) args[0], f->esp);
      f->eax = exec((const char *) args[0]);
      break;
    case SYS_REMOVE:
      copy_arguments(f->esp, args, 1);
      f->eax = remove((const char *) args[0]);
      break;
    case SYS_FILESIZE:
      copy_arguments(f->esp, args, 1);
      f->eax = filesize(args[0]);
      break;
    case SYS_SEEK:
      copy_arguments(f->esp, args, 2);
      seek(args[0], (unsigned) args[1]);
      break;
    case SYS_TELL:
      copy_arguments(f->esp, args, 1);
      f->eax = tell(args[0]);
      break;
  }

  // printf ("system call!\n");
  // thread_exit ();
}

/* Once succeeded, return mapping id, otherwise, return -1. 
   Load file data into memory by demanding paging.
   fd: mapping file to virtual space of process.
   addr: starting mapping address. */
int mmap (int fd, void *addr) {
  /* Check arguments:
        Error: return error
        Valid: file_reopen()
               allocate mmId
               create mm_file & initialize it
               create vm_entry & initialize it
               return mmId */
  if (pg_ofs(addr) != 0 || !addr)
    return -1;
  if (is_user_vaddr(addr) == false)
    return -1;

  struct thread *cur = thread_current();
  struct file *file = file_reopen(get_file_descriptor(fd));
  struct mm_file *mm_file;
  size_t ofs = 0;
  int length;

  mm_file = (struct mm_file *) malloc(sizeof(struct mm_file));
  memset(mm_file, 0, sizeof(struct mm_file));
  mm_file->file = file;
  mm_file->mmId = cur->next_mmId++;
  list_init (&mm_file->vmE_list);
  list_push_back (&cur->mm_list, &mm_file->elem);

  length = file_length(mm_file->file);
  while (length > 0) {
    if (find_vm_entry(addr))
      return -1;
    
    struct vm_entry *vmE = (struct vm_entry *) malloc(sizeof(struct vm_entry));
    memset(vmE, 0, sizeof(struct vm_entry));
    vmE->type = VM_FILE;
    vmE->writable_flag = true;
    vmE->vaddr = addr;
    vmE->offset = ofs;
    vmE->zero_bytes = 0;
    vmE->file = mm_file->file;
    if (length < PGSIZE)
      vmE->read_bytes = length;
    else
      vmE->read_bytes = PGSIZE;

    list_push_back (&mm_file->vmE_list, &vmE->mm_elem);
    insert_vm_entry (&cur->vm, vmE);

    addr += PGSIZE;
    ofs += PGSIZE;
    length -= PGSIZE;
  }
  return mm_file->mmId;
}

/* Clear all of vm_entry w.r.t mmId in mm_list. When removing mapping, call do_munmap(). */
void munmap (int _mmId) {
  /* Iterate mm_list:
        mmId dismatch: iterate
        mmId match: remove vm_entry
                    remove mm_file
                    file_close. */
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->mm_list); e != list_end (&thread_current ()->mm_list); e = list_next (e)) {
      struct mm_file *_mm_file = list_entry (e, struct mm_file, elem);
      if (_mm_file->mmId == _mmId) {
        do_munmap(_mm_file);
      }
  }
}

/* Remove all of vm_entry connected with vmE_list of mm_file. If physical page w.r.t. virtual address to vm_entry
   exists and dirty, record memory contents on disk. */
void do_munmap (struct mm_file* _mm_file) {
  ASSERT (_mm_file != NULL);

  struct list_elem *e;
  for (e = list_begin (&_mm_file->vmE_list); e != list_end (&_mm_file->vmE_list); ) {
      struct vm_entry *vmE = list_entry (e, struct vm_entry, mm_elem);
      if (vmE->load_flag && pagedir_is_dirty(thread_current()->pagedir, vmE->vaddr)) {
          if (file_write_at (vmE->file, vmE->vaddr, vmE->read_bytes, vmE->offset) != (int) vmE->read_bytes)
              NOT_REACHED ();
          // free_page (pagedir_get_page (thread_current()->pagedir, vmE->vaddr));
      }
      vmE->load_flag = false;
      e = list_remove (e);
      delete_vm_entry (&thread_current()->vm, vmE);
  }
  list_remove (&_mm_file->elem);
  free (_mm_file);
}