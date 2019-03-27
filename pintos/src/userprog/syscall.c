#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.c"

static void syscall_handler (struct intr_frame *);

void* valid_pointer(void* ptr)
{
	if(!is_user_vaddr(ptr))
	{
		//exit with status -1
		return 0;
	} 
	if(!pagedir_get_page(thread_current()->pagedir, ptr))
	{
		//exit with status -1
		return 0;
	}
	return ptr;
}

void
exit_with_status(void)

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}
