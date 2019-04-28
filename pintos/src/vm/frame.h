#ifndef VM_FRAME_H
#define VM_FRAME_H


struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;
	struct list_elem elem;
};

void frame_init (void);
bool allocate_frame (void *addr, struct sup_page_table_entry* spte);

#endif /* vm/frame.h */
