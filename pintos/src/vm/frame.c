#include "vm/frame.h"
#include "threads/init.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "vm/page.h"
#include <stdbool.h>
#include <stdio.h>

/*
 * Initialize frame table
 */
void 
frame_init (void)
{
	list_init(&frame_table);
	printf("frame_init\n");
}


/* 
 * Make a new frame table entry for addr.
 */
bool
allocate_frame (void *addr, struct sup_page_table_entry* spte)
{
	if(list_size(&frame_table)>=1<<20)
		return 0;
	struct frame_table_entry* fte;
	fte -> frame = addr;
	fte -> owner = thread_current();
	fte -> spte = spte;
	list_push_front(&frame_table, &(fte->elem));
	printf("alloc_frame\n");
	return 1;

}

