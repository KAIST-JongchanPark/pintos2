#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include <stdbool.h>

/*
 * Initialize frame table
 */
void frame_init (void)
{
	list_init(&frame_table);
}


/* 
 * Make a new frame table entry for addr.
 */
void allocate_frame (void *kpage, void* upage)
{
	struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));
	
	fte -> kpage = (void *)kpage;
	fte -> owner = thread_current();
	fte -> upage = (void *)(((uintptr_t)upage >> 12) << 12);
	fte -> counter = 0;
	fte -> dirty = pagedir_is_dirty(thread_current()->pagedir, upage)||pagedir_is_dirty(thread_current()->pagedir, kpage);
	fte -> accessed = pagedir_is_accessed(thread_current()->pagedir, upage)||pagedir_is_accessed(thread_current()->pagedir, kpage);
	
	list_push_front(&frame_table, &(fte->elem));
}

void free_frame (void *addr)
{

	struct list_elem *target_elem = frame_find_addr(&frame_table, (void *)addr);
	if(target_elem==NULL)
	{
		PANIC("target_elem is null");
	}
	printf("f1\n");
	struct frame_table_entry *target_entry = malloc(sizeof(struct frame_table_entry));
	printf("f2\n");
	target_entry = list_entry (target_elem, struct frame_table_entry, elem);
	printf("f3\n");
	//free(target_entry);
	printf("f4\n");
	list_remove(target_elem);
	printf("f5\n");
	//free(target_entry);
}

static inline bool
is_tail (struct list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

struct list_elem *frame_find_addr (struct list *list, void *addr) // not work?
{
  struct list_elem *curr_elem = list_front (list);
  while(!is_tail(curr_elem)) 
  {
      if((list_entry (curr_elem, struct frame_table_entry, elem)->kpage) == addr)
	  {
        return curr_elem;
	  }
	  curr_elem = list_next(curr_elem);
	  printf("hello\n");
  }
  //printf("hello\n");
  return NULL;
}

struct frame_table_entry* find_frame_to_evict(void)
{
	struct list_elem* target_elem = list_pop_max(&frame_table);
	struct frame_table_entry* fte = list_entry(target_elem, struct frame_table_entry, elem);
	return fte;
}


struct list_elem *
list_pop_max (struct list *list)
{
  struct list_elem *max_elem = list_front (list);
  struct list_elem *curr_elem = list_front (list);
  while(!is_tail(curr_elem)) 
  {
      if((list_entry (max_elem, struct frame_table_entry, elem) -> counter) < (list_entry (curr_elem, struct frame_table_entry, elem) -> counter))
	  {
        max_elem = curr_elem;
	  }
	  curr_elem = list_next(curr_elem);
  }
  list_remove(max_elem);
  return max_elem;
}   


