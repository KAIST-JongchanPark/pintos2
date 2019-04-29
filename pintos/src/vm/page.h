#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/hash.h"

struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	/*
	uint64_t access_time;

	bool dirty;
	bool accessed;
	*/
	struct hash_elem elem;

};

void spt_init (struct hash spt);
void allocate_spt (struct hash spt, void *addr);
void destroy_spt (struct hash spt);

#endif /* vm/page.h */
