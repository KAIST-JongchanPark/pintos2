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
#include "vm/page.h"
#include "filesys/off_t.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"


typedef int pid_t;
typedef int mapid_t;
struct lock syscall_lock;

void munmap (mapid_t mapping);
mapid_t mmap (int fd, void *addr);
struct list_elem *mmap_list_find_mapid (struct list *list, mapid_t mapid);
struct sup_page_table_entry* mapping_to_spte(mapid_t mapping);
struct mmap_elem
{
  struct list_elem elem;
  mapid_t mapid;
  off_t size;
  struct file* file;
  void* vaddr;
};


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
	if(!is_user_vaddr(ptr)||ptr==NULL)
	{
			//exit with status -1
		exit_with_status(-1);
		return 0;
	} 
	if(!lookup_spt(ptr))
	{
		//exit with status -1
		//printf("not spt addr: %x\n", ptr);
		//printf("not spt addr: %x\n", ptr);
		int *temp = ptr+PGSIZE;
		//printf("what is temp: %d\n", *temp);
		while(is_user_vaddr(temp)&&!lookup_spt(temp))
		{
			temp+=PGSIZE;
			//printf("not spt calc addr: %x\n", temp);
		}
		//printf("what is temp: %d\n", *temp);
		if(is_user_vaddr(temp))
		{
			exit_with_status(-1);
			//return -1;
		}
		return ptr;
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
    if(thread_current()->fd[i]!=NULL)
    {
      file_close(thread_current()->fd[i]);
      thread_current()->fd[i] = NULL;
    }
    
  }
  struct list *mmap_list = &thread_current()->mmap_list;
  while (!list_empty(mmap_list)) {
    struct list_elem *m_elem = list_begin (mmap_list);
    struct mmap_elem *mme = list_entry(m_elem, struct mmap_elem, elem);

    // in sys_munmap(), the element is removed from the list
    munmap (mme->mapid);
  }
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
  //struct sup_page_table_entry *spte = spt_get_page(ptr);
  //ASSERT(spte!=NULL);
  //printf("open ptr: %x\n", ptr);
  //spte -> file = file;
  //spte -> type = DISK;
  //ASSERT(spt_get_page(ptr)->type==DISK);
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
	//printf("test1");
    return -1;
  //lock_acquire(&syscall_lock);
  //if(!lookup_spt(buffer)&&pg_ofs(buffer)!=0)
  //{
	  //printf("error addr: %x\n", buffer);
	  //printf("stack addr: %x\n", thread_current()->stack);
	  //exit_with_status(-1);
  //}
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
	  //printf("test2");
      return -1;
    }

    void* upage = pg_round_down(buffer);
    while(upage<buffer+size)
    {
      allocate_and_init_to_zero(upage);
      upage+=PGSIZE;
    }

    return_value = file_read(file, buffer, (off_t)size);


    //lock_release(&syscall_lock);
    return return_value;
  }
  else
  {
    //lock_release(&syscall_lock);
	//printf("test3");
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

mapid_t mmap (int fd, void *addr)
{
  //PANIC("mmap syscall test");
  //invalid fd
  if (fd<0||fd>=128||fd==0||fd==1)
  {
	//PANIC("mmap syscall test1");
    return -1;
  }
  //invalid addr
  if (addr==NULL||addr>=PHYS_BASE-0x800000||addr<0x08048000)
  {
	//PANIC("mmap syscall test2");
    return -1;
  }
  //file's size 0
  
  struct file* file = thread_current()->fd[fd];
  struct file* refile = file_reopen(file);
  //PANIC("mmap syscall test");
  off_t size = file_length(refile);
  
  if (size==0)
  {
	//PANIC("mmap syscall test3");
    return -1;
  }
  //conflicting with existing address region
  if (pg_round_down(addr)!=addr)
  {
    return -1;
  }
  off_t checker = 0;
  while(checker<size)
  {
    if(lookup_spt(addr+checker))
	{
	  //PANIC("mmap syscall test4");
      return -1;
	}
    checker+=PGSIZE;
  }
  //mmap with lazy loading => used in 
  
  uint32_t read_bytes, zero_bytes;
  off_t ofs = 0;
  read_bytes = size;
  zero_bytes = PGSIZE - size%PGSIZE;

  file_seek(refile,0);
  mapid_t id = thread_current()->mapid+1;
  thread_current()->mapid+=1;
  struct mmap_elem* mme = malloc(sizeof(struct mmap_elem));
  mme->mapid = id;
  mme->vaddr = addr;
  mme->file = refile;
  mme->size = size;
  list_push_back(&(thread_current()->mmap_list), &(mme->elem));
  
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
    
      struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));
      //spte -> page = lookup_page(t->pagedir, upage, false);
      spte -> page_vaddr = (void *)(((uintptr_t)addr >> 12) << 12);
      spte -> file = refile;
      spte -> ofs = ofs;
      spte -> writable = true;
      spte -> read_bytes = page_read_bytes;
      spte -> type = FILE;
      spte -> mapid = id;

      spte -> swapped = false;
      
      allocate_spt(thread_current()->spt, spte);
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      addr += PGSIZE;
      ofs += PGSIZE;
    }
  //printf("return id: %d\n", id);
  return id;
}

static inline bool
is_tail (struct list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

struct list_elem *mmap_list_find_mapid (struct list *list, mapid_t mapid)
{
  if(list_empty(list))
  {
	  return NULL;
  }
  struct list_elem *curr_elem = list_front (list);
  while(!is_tail(curr_elem)) 
  {
      if(list_entry (curr_elem, struct mmap_elem, elem)->mapid == mapid)
    {
        return curr_elem;
    }
    curr_elem = list_next(curr_elem);
  }
  return NULL;
}

struct sup_page_table_entry* mapping_to_spte(mapid_t mapping)
{
    struct list_elem * m_elem = mmap_list_find_mapid(&(thread_current()->mmap_list), mapping);
    if(m_elem==NULL)
      return NULL;
    struct mmap_elem* mme = list_entry(m_elem, struct mmap_elem, elem);
    struct sup_page_table_entry *spte = spt_get_page(mme->vaddr);
	list_remove(m_elem);
    return spte;
}

void munmap (mapid_t mapping)
{
    //iterate through spt, with comparing mapid
    
    struct sup_page_table_entry *spte = mapping_to_spte(mapping);

	if(spte == NULL)
	{
		PANIC("spte is null.");
	}
    while(spte!=NULL)
    {
       file_seek(spte->file, spte->ofs);
       //if(pagedir_is_dirty(thread_current()->pagedir, spte->page_vaddr)||pagedir_is_dirty(thread_current()->pagedir, pagedir_get_page(thread_current()->pagedir, spte->page_vaddr)))
        file_write(spte->file, spte->page_vaddr, spte->read_bytes);
	   void *kpage = pagedir_get_page(thread_current()->pagedir, spte->page_vaddr);
	   if(kpage!=NULL)
	   {
			//palloc_free_page(kpage);
			//free_frame(kpage);
	   }
       free_spt(spte);
       spte = mapping_to_spte(mapping);
	     
    }
    //for each element, remove from spt and free it. 

    //when freeing it, we should write content of the page back to disk. 
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
      //is_valid_ptr((void *)*(int *)(f->esp+4));
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
    case SYS_MMAP:
      is_valid_ptr((void *)(f->esp+4));
      is_valid_ptr((void *)(f->esp+8));
      //is_valid_ptr((void *)*(int *)(f->esp+8));
      f->eax = mmap(*(int *)(f->esp+4), (void *)*(int *)(f->esp + 8));
      break;
    case SYS_MUNMAP:   
      is_valid_ptr((void *)(f->esp+4));
	  munmap(*(mapid_t *)(f->esp+4));
      //f->eax = munmap(*(int *)(f->esp+4));
      break;
  }
 }