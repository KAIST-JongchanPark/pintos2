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
#include "threads/synch.h"

typedef int pid_t;
struct lock syscall_lock;

struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };


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
  //lock_init(&syscall_lock);
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
  if(ptr==NULL)
  {
    exit_with_status(-1);
  }
  return filesys_create(ptr, size);
}

bool remove (const char *ptr)
{
  return filesys_remove(ptr);
}

int open (const char *ptr)
{
  if(ptr==NULL)
    return -1;
  //lock_acquire(&syscall_lock);
  struct file* file = filesys_open(ptr);
  if(file==NULL)
  {
    //lock_release(&syscall_lock);
    return -1;
  }
  
  int i;
  for(i=3;i<128;i++)
  {
    if(thread_current()->fd[i]==NULL)
    {
	  if (strcmp(thread_current()->name, ptr) == 0)
	  {
		file_deny_write(file);  
	  }
      thread_current()->fd[i] = file;
      //lock_release(&syscall_lock);
      return i;
    }
  }
  //lock_release(&syscall_lock);
  return -1;
}

int filesize (int fd)
{
  if(fd<0||fd>=128)
    return -1;
  struct file* file = thread_current ()->fd[fd];
  if(file==NULL)
    return -1;
  return file_length(file);
}

int read (int fd, void *buffer, unsigned size)
{
  int return_value;
  if(fd<0||fd>=128)
    return -1;
  //lock_acquire(&syscall_lock);
  if(fd==0)
  {
    int i;
    for(i=0;i<size;i++)
    {
      ((uint8_t *)buffer)[i] = input_getc();
    }
    //lock_release(&syscall_lock);
    return size;
  } 
  else if(fd>2)
  {
    struct file* file = thread_current ()->fd[fd];
    if(file==NULL)
    {
      //lock_release(&syscall_lock);
      return -1;
    }
    return_value = file_read(file, buffer, (off_t)size);
    //lock_release(&syscall_lock);
    return return_value;
  }
  else
  {
    //lock_release(&syscall_lock);
    return -1;
  }
}

int write (int fd, const void *buffer, unsigned size) 
{
  int return_value;
  if(fd<0||fd>=128)
    return -1;
  //lock_acquire(&syscall_lock);
  if (fd == 1) 
  {
    putbuf(buffer, size);
    //lock_release(&syscall_lock);
    return size;
  }
  else if (fd>2)
  {
    struct file* file = thread_current()->fd[fd];
    if (file==NULL)
    {
      //lock_release(&syscall_lock);
      return -1;
    }
	if(file->deny_write)
	{
	  //file_deny_write(file);
	  //lock_release(&syscall_lock);
	  return 0;
	}
    return_value = file_write(file, buffer,(off_t)size);
    //lock_release(&syscall_lock);
    return return_value;
  }
  //lock_release(&syscall_lock);
  return -1;
}

void seek (int fd, unsigned position)
{
  struct file* file = thread_current()->fd[fd];
  file_seek(file, (off_t)position);
}

unsigned tell (int fd)
{
  struct file* file = thread_current()->fd[fd];
  return file_tell(file);
}

void close(int fd)
{
  if (fd<0||fd>=128)
    return;
  struct file* file = thread_current()->fd[fd];
  if (file==NULL)
    return;
  file_close(thread_current()->fd[fd]);
  thread_current()->fd[fd] = NULL;
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
      is_valid_ptr((void *)*(uint32_t *)(f->esp+4));
      f->eax = exec((const char *)*(uint32_t *)(f->esp+4));
    	break;                   /* Start another process. */
    case SYS_WAIT:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = wait((pid_t)*(uint32_t *)(f->esp+4));
    	break;                   /* Wait for a child process to die. */
    case SYS_CREATE:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)*(uint32_t *)(f->esp+4));
      f->eax = create((const char *)*(uint32_t *)(f->esp+4), (off_t)*(unsigned *)(f->esp+8));
    	break;                 /* Create a file. */
    case SYS_REMOVE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = remove((const char *)*(uint32_t *)(f->esp+4));
    	break;                 /* Delete a file. */
    case SYS_OPEN:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)*(uint32_t *)(f->esp+4));
      f->eax = open((const char *)*(uint32_t *)(f->esp+4));
    	break;                   /* Open a file. */
    case SYS_FILESIZE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = filesize((int)*(uint32_t *)(f->esp+4));
    	break;               /* Obtain a file's size. */
    case SYS_READ:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
      is_valid_ptr((void *)*(uint32_t *)(f->esp+8));
      ////lock_acquire(&syscall_lock);
      f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    	////lock_release(&syscall_lock);
      break;                   /* Read from a file. */
    case SYS_WRITE:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
      is_valid_ptr((void *)*(uint32_t *)(f->esp+8));
		  f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    	break;                  /* Write to a file. */
    case SYS_SEEK:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      seek((int)*(uint32_t *)(f->esp+4), (unsigned)*(uint32_t *)(f->esp+8));
    	break;                   /* Change position in a file. */
    case SYS_TELL:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = tell((int)*(uint32_t *)(f->esp+4));
    	break;                   /* Report current position in a file. */
    case SYS_CLOSE:
      is_valid_ptr((void *)(f->esp+4));
      close((int)*(uint32_t *)(f->esp+4));
    	break;     
  }
 }
