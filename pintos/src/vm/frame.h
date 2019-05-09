#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "lib/kernel/list.h"
#include "threads/thread.h"
#include <stdbool.h>

struct frame_table_entry
{
	void* kpage;
	void* upage;
	struct thread *owner;
	bool dirty;
	bool accessed;
	//struct sup_page_table_entry* spte;
	struct list_elem elem;
	int counter;
};

void frame_init (void);
void allocate_frame (void *kpage, void* upage);
void free_frame (void *addr);
struct list_elem *frame_find_addr (struct list *list, void *addr);
struct frame_table_entry* find_frame_to_evict(void);
struct list_elem *list_pop_max (struct list *list);


#endif /* vm/frame.h */
