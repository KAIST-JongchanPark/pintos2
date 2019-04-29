#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"

struct list_elem * frame_find_addr (struct list *list, void *addr);

/*
 * Initialize frame table
 */
void 
frame_init (void)
{
	list_init(frame_table);
}


/* 
 * Make a new frame table entry for addr.
 */
void
allocate_frame (void *addr)
{
	struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));
	
	fte -> frame = (void *)vtop(addr);
	fte -> owner = thread_current();
	
	list_push_front(frame_table, &(fte->elem));
}

void
free_frame (void *addr)
{
	struct list_elem *target_elem = frame_find_addr(frame_table, (void *)vtop(addr));
	struct frame_table_entry *target_entry = list_entry (curr_elem, struct frame_table_entry, elem);
	list_remove(target_elem);
	free(target_entry);
}

struct list_elem *
frame_find_addr (struct list *list, void *addr)
{
  struct list_elem *curr_elem = list_front (list);
  while(!is_tail(curr_elem)) 
  {
      if(list_entry (curr_elem, struct frame_table_entry, elem)->frame == addr)
	  {
        return target_elem;
	  }
	  curr_elem = list_next(curr_elem);
  }
  return NULL;
}
