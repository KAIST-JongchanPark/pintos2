#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include <stdbool.h>
#include "userprog/syscall.h"
#include "threads/thread.h"


enum page_type
{
	DISK,
	HEAP,
	FILE
};



struct sup_page_table_entry 
{
	void *page_vaddr;
	/*
	uint64_t access_time;
	*/
	bool accessed;
	bool swapped;
	
	struct thread *thread;
	
	struct hash_elem elem;
	struct file* file;
	off_t ofs;
	bool writable;
    size_t read_bytes;
	enum page_type type;
	int mapid;
	size_t swapped_place;

};

struct hash *spt_init (void);
void allocate_spt (struct hash *spt, struct sup_page_table_entry *spte);
void free_spt (struct sup_page_table_entry* spte);
void destroy_spt (struct hash *spt);
bool allocate_and_init_to_zero(void* addr);
bool allocate_using_spt(void* addr, struct sup_page_table_entry *spte);
bool lookup_spt(void* addr);
struct sup_page_table_entry *spt_get_file_mapping(mapid_t);
struct sup_page_table_entry *spt_get_page(void *addr);


#endif /* vm/page.h */
