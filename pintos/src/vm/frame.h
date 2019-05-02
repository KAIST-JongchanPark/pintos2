#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "lib/kernel/list.h"
#include "threads/thread.h"

struct frame_table_entry
{
	void *frame;
	struct thread *owner;
	//struct sup_page_table_entry* spte;
	struct list_elem elem;
};

void frame_init (void);
void allocate_frame (void *addr);
void free_frame (void *addr);
struct list_elem *frame_find_addr (struct list *list, void *addr);

#endif /* vm/frame.h */
