#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/init.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

typedef int pid_t;

static void syscall_handler (struct intr_frame *);
pid_t exec (const char *);

void* is_valid_ptr(void* ptr)
{
	if(!is_user_vaddr(ptr))
	{
			//exit with status -1
		exit_with_status(-1);
		return 0;
	} 
	if(!pagedir_get_page(thread_current()->pagedir, ptr))
	{
		//exit with status -1
		exit_with_status(-1);
		return 0;
	}
	return ptr;
}

void
exit_with_status(int status)
{
  thread_current()->exit_status = status;
	thread_exit ();
	//exit with given status => further used in process.c(when printing results)
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt (void) 
{
  power_off();
}

void exit (int status) 
{
  exit_with_status(status);
}

pid_t exec (const char *file)
{
  return process_execute(file);
}

int wait(tid_t tid)
{
  return process_wait(tid);
}

bool create (const char *ptr, unsigned size)
{
  return filesys_create(ptr, size);
}

bool remove (const char *ptr)
{
  return filesys_remove(ptr);
}


int read (int fd, void *buffer, unsigned size)
{
  if(fd==0)
  {
    int i;
    for(i=0;i<size;i++)
    {
      ((uint8_t *)buffer)[i] = input_getc();
    }
    return size;
  } 
  else 
  {
    return 0;
  }
}

int write (int fd, const void *buffer, unsigned size) 
{
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}


static void
syscall_handler (struct intr_frame *f) 
{
  //printf ("system call!\n");
  
  int *syscall_ptr = f->esp;
  is_valid_ptr(syscall_ptr);
  int syscall_number = *syscall_ptr;
  //printf("syscall executed");
  //hex_dump(syscall_ptr, syscall_ptr, 100, 1);
  //thread_exit ();
  switch(syscall_number) 
  {
  	case SYS_HALT:
  		halt();
  		break;
  	case SYS_EXIT:
  		//find status//
  		exit(*(int*)is_valid_ptr((void*)(syscall_ptr+1)));
  		break;                   /* Terminate this process. */
    case SYS_EXEC:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = exec((const char *)*(uint32_t *)(f->esp+4));
    	break;                   /* Start another process. */
    case SYS_WAIT:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = wait((tid_t)*(uint32_t *)(f->esp+4));
    	break;                   /* Wait for a child process to die. */
    case SYS_CREATE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = create((const char *)*(uint32_t *)(f->esp+4), (off_t)*(unsigned *)(f->esp+8));
    	break;                 /* Create a file. */
    case SYS_REMOVE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = remove((const char *)*(uint32_t *)(f->esp+4));
    	break;                 /* Delete a file. */
    case SYS_OPEN:
    	break;                   /* Open a file. */
    case SYS_FILESIZE:
      is_valid_ptr((void *)(f->esp+4));
    	break;               /* Obtain a file's size. */
    case SYS_READ:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
      f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    	break;                   /* Read from a file. */
    case SYS_WRITE:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
		  f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    	break;                  /* Write to a file. */
    case SYS_SEEK:
    	break;                   /* Change position in a file. */
    case SYS_TELL:
    	break;                   /* Report current position in a file. */
    case SYS_CLOSE:
    	break;     
  }
 }
