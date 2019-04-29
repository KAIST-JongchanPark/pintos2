#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "lib/kernel/list.h"

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

#endif /* vm/frame.h */
