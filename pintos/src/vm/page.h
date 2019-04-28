#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/hash.h"
#include "threads/thread.h"

struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	uint64_t access_time;

	bool dirty;
	bool accessed;

	struct hash_elem elem;
};

void page_init (struct thread *);
struct sup_page_table_entry *allocate_page (void *addr);

#endif /* vm/page.h */
