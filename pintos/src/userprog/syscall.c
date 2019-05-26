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
#include "filesys/inode.h"

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
  int i;
  thread_current()->exit_status = status;
  for(i=3;i<128;i++)
  {
    if(thread_current()->sfde[i]->allocated==true)
    {
      if(thread_current()->sfde[i]->isdir){
        dir_close(thread_current()->sfde[i]->dir);
      }
      else{
        file_close(thread_current()->sfde[i]->file);
      }
      //thread_current()->fd[i] = NULL;
      thread_current()->sfde[i] = NULL;
    }
    free(sfde);
      
  }
	thread_exit ();
	//exit with given status => further used in process.c(when printing results)
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  int i;
  struct sup_fd_entry *sfde
  for(i=0;i<128;i++)
  {
    sfde = malloc(sizeof(struct sup_fd_entry));
    sfde->allocated = false;
    sfed->isdir = false;
    sfde->dir = NULL;
    sfde->file = NULL;
    thread_current()->sfde[i] = sfde;
  }
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
  return filesys_create(ptr, size, false);
}

bool remove (const char *ptr)
{

  return filesys_remove(ptr, is_dir(ptr));
}

int open (const char *ptr)
{
  if(ptr==NULL)
    return -1;
  //lock_acquire(&syscall_lock);
  if(!is_dir(ptr))
  {
    struct file* file = filesys_open(ptr);
    if(file==NULL)
    {
      //lock_release(&syscall_lock);
      return -1;
    }
    
    int i;
    for(i=3;i<128;i++)
    {
      if(thread_current()->sfde[i]->allocated == false)
      {
      if (strcmp(thread_current()->name, ptr) == 0)
      {
      file_deny_write(file);  
      }
        struct sup_fd_entry* sfde;
        sfde->isdir = false;
        sfde->file = file;
        sfde->dir = NULL;
        sfde->allocated = true;
        thread_current()->sfde[i] = sfde;
        //lock_release(&syscall_lock);
        return i;
      }
    }
    //lock_release(&syscall_lock);
    return -1;
  }
  else
  {
    struct dir* parent_dir = get_parent_dir(ptr);
    struct inode* inode;
    if(!dir_lookup(parent_dir, get_name(ptr), &inode))
      return -1;

    struct dir* target_dir = dir_open(inode);
    if(target_dir==NULL)
      return -1;

    int i;
    for(i=3;i<128;i++)
    {
      if(thread_current()->sfde[i]==NULL)
      {
      
        struct sup_fd_entry* sfde;
        sfde->isdir = true;
        sfde->file = NULL;
        sfde->dir = dir;
        sfde->allocated = true;
        thread_current()->sfde[i] = sfde;
        //lock_release(&syscall_lock);
        return i;
      }
    }
    //lock_release(&syscall_lock);
    return -1;



  }
}

int filesize (int fd)
{
  if(fd<0||fd>=128)
    return -1;
  struct file* file = thread_current ()->sfde[fd]->file;
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

    struct file* file = thread_current ()->sfde[fd]->file;
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
    struct file* file = thread_current()->sfde[fd]->file;
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
  struct file* file = thread_current()->sfde[fd]->file;
  if(file==NULL)
    return;
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
  struct sup_fd_entry* sfde = thread_current()->sfde[fd];
  if(sfde->allocated == false)
    return;

  if(!sfde->isdir)
  {
    struct file* file = sfde->file;
    file_close(file);
    sfde->allocated = false;
    sfde->file = NULL;
  }
  else
  {
    struct dir* dir = sfde->dir;
    dir_close(dir)
    sfde->allocated = false;
    sfde->dir = NULL;
  }
}

bool is_file(char* ptr)
{
  struct file* file = filesys_open (ptr);
  if (file == NULL) 
    {
      return false;
    }
  file_close(file);
  return true;
}

bool is_dir(char *ptr)
{
  char* copy;
  char* ret_ptr;
  char* next_ptr;
  strcpy(copy, ptr);
  ret_ptr = strtok_r(ptr, "/", &next_ptr);
  if(ret_ptr==NULL)
  {
    return !is_file(copy);
  }
  while(ret_ptr!=NULL)
  {
    if(is_file(ret_ptr))
      return false;
    ret_ptr = strtok_r(NULL, "/", &next_ptr);
    /*
    if(is_file(ret_ptr))
      return false;
      */
  }
  return true;

}

struct dir* chdir_parse_dir(char* dir)
{
  if(!is_dir(dir))
    PANIC("Not a directory");
  char* dir_copy;
  strcpy(dir_copy, dir);
  struct dir* current_dir;
  char* ret_ptr;
  char* next_ptr;
  struct inode* inode;

  if(dir[0]=='/')
  {
    current_dir = dir_open_root();
  }
  else
  {
    current_dir = thread_current()->dir;
    
  }

  ret_ptr = strtok_r(ptr, "/", &next_ptr);
  while(ret_ptr!=NULL)
  {
    if(!dir_lookup(current_dir, ret_ptr, &inode))
      return NULL;
    current_dir = dir_open(inode);
    if(current_dir==NULL)
    {
      return NULL;
    }
    ret_ptr = strtok_r(NULL, "/", &next_ptr);
  }
  current_dir = dir_open(inode);
  return current_dir;
}

void* mkdir_parse_dir(char* dir, int option)
{
  if(!is_dir(dir))
    PANIC("Not a directory");
  char* dir_copy;
  strcpy(dir_copy, dir);
  struct dir* current_dir;
  char* ret_ptr;
  char* next_ptr;
  struct inode* inode;

  if(dir[0]=='/')
  {
    current_dir = dir_open_root();
  }
  else
  {
    current_dir = thread_current()->dir;
    
  }

  ret_ptr = strtok_r(dir, "/", &next_ptr);
  while(ret_ptr!=NULL)
  {
    if(!dir_lookup(current_dir, ret_ptr, &inode))
      return NULL;
    current_dir = dir_open(inode);
    if(current_dir==NULL)
    {
      return NULL;
    }
    ret_ptr = strtok_r(NULL, "/", &next_ptr);
  }
  if(option==1)
  {
    return (void *)current_dir;
  }
  else
  {
    return (void *)next_ptr;
  }
  
}

bool chdir (const char *dir)
{
  struct dir* target_dir = chdir_parse_dir(dir);
  if(target_dir==NULL)
    return false;
  thread_current()->dir = target_dir;
  return true;
}

bool mkdir (const char *dir)
{
  struct dir* parent_dir = (struct dir*) mkdir_parse_dir(dir, 1);
  char *dir_name = (char *) mkdir_parse_dir(dir, 2);
  return filesys_create(dir_name,0 ,true);
}

bool readdir (int fd, char *name)
{
  struct sup_fd_entry* sfde = thread_current()->sfde[fd];
  if(sfde->allocated == false)
    return false;
  if(!sfde->isdir)
    return false;
  return dir_readdir(sfde->dir, name)
}

//
bool isdir (int fd)
{
  struct sup_fd_entry* sfde = thread_current()->sfde[fd];
  if(sfde->allocated == false)
    return false;
  return sfde->isdir;
}

int inumber (int fd)
{
  struct sup_fd_entry*  sfde = sup_fd_list[fd];
  struct dir* dir = sfde->dir;
  return (int) inode_get_inumber(dir_get_inode(dir));
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
      is_valid_ptr((void *)*(int *)(f->esp+4));
      f->eax = exec((const char *)*(int *)(f->esp+4));
    	break;                   /* Start another process. */
    case SYS_WAIT:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = wait((pid_t)*(int *)(f->esp+4));
    	break;                   /* Wait for a child process to die. */
    case SYS_CREATE:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)*(int *)(f->esp+4));
      f->eax = create((const char *)*(int *)(f->esp+4), (off_t)*(unsigned *)(f->esp+8));
    	break;                 /* Create a file. */
    case SYS_REMOVE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = remove((const char *)*(int *)(f->esp+4));
    	break;                 /* Delete a file. */
    case SYS_OPEN:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)*(int *)(f->esp+4));
      f->eax = open((const char *)*(int *)(f->esp+4));
    	break;                   /* Open a file. */
    case SYS_FILESIZE:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = filesize(*(int *)(f->esp+4));
    	break;               /* Obtain a file's size. */
    case SYS_READ:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
      is_valid_ptr((void *)*(int *)(f->esp+8));
      ////lock_acquire(&syscall_lock);
      f->eax = read(*(int *)(f->esp+4), (void *)*(int *)(f->esp + 8), (unsigned)*((int *)(f->esp + 12)));
    	////lock_release(&syscall_lock);
      break;                   /* Read from a file. */
    case SYS_WRITE:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      is_valid_ptr((void *)(f->esp+12));
      is_valid_ptr((void *)*(int *)(f->esp+8));
		  f->eax = write(*(int *)(f->esp+4), (void *)*(int *)(f->esp + 8), (unsigned)*((int *)(f->esp + 12)));
    	break;                  /* Write to a file. */
    case SYS_SEEK:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      seek(*(int *)(f->esp+4), (unsigned)*(int *)(f->esp+8));
    	break;                   /* Change position in a file. */
    case SYS_TELL:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = tell(*(int *)(f->esp+4));
    	break;                   /* Report current position in a file. */
    case SYS_CLOSE:
      is_valid_ptr((void *)(f->esp+4));
      close(*(int *)(f->esp+4));
    	break;     
    case SYS_CHDIR:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = chdir((char *)(f->esp+4));
    case SYS_MKDIR:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = mkdir((char *)(f->esp+4));
    case SYS_READDIR:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      f->eax = readdir(*(int *)(f->esp+4), (char *)(f->esp+8));
    case SYS_ISDIR:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = isdir(*(int *)(f->esp+4));
    case SYS_INUMBER:
      is_valid_ptr((void *)(f->esp+4));
      f->eax = inumber(*(int *)(f->esp+4));
  }
 }
