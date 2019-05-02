#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/hash.h"


enum page_type{
	DISK
	STACK
	HEAP
};



struct sup_page_table_entry 
{
	void* user_vaddr;
	/*
	uint64_t access_time;

	bool dirty;
	bool accessed;
	*/
	struct hash_elem elem;
	struct file* file;
	off_t ofs;
    uint32_t read_bytes;
	enum page_type type;

};

struct hash *spt_init (void);
void allocate_spt (struct hash *spt, void *addr);
void destroy_spt (struct hash *spt);

#endif /* vm/page.h */
