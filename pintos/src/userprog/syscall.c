#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/init.h"

static void syscall_handler (struct intr_frame *);

static void* is_valid_ptr(void* ptr)
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

static void
exit_with_status(int status)
{
	//exit with given status => further used in process.c(when printing results)
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  hex_dump(*f->esp, *f->esp, 100, 1);
  printf ("system call!\n");
  thread_exit ();
  
  int *syscall_ptr = f->esp;
  is_valid_ptr(syscall_ptr);
  int syscall_number = *syscall_ptr;
  printf("syscall executed");
  switch(syscall_number) 
  {
  	case SYS_HALT:
  		power_off();
  		break;
  	case SYS_EXIT:
  		//find status
  		exit_with_status(*(int*)is_valid_ptr((void*)(syscall_ptr+1)));
  		break;                   /* Terminate this process. */
    case SYS_EXEC:
    	break;                   /* Start another process. */
    case SYS_WAIT:
    	break;                   /* Wait for a child process to die. */
    case SYS_CREATE:
    	break;                 /* Create a file. */
    case SYS_REMOVE:
    	break;                 /* Delete a file. */
    case SYS_OPEN:
    	break;                   /* Open a file. */
    case SYS_FILESIZE:
    	break;               /* Obtain a file's size. */
    case SYS_READ:
    	break;                   /* Read from a file. */
    case SYS_WRITE:
    	break;                  /* Write to a file. */
    case SYS_SEEK:
    	break;                   /* Change position in a file. */
    case SYS_TELL:
    	break;                   /* Report current position in a file. */
    case SYS_CLOSE:
    	break;     
  }
 }
