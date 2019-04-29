#include "vm/page.h"
#include "lib/kernel/hash.h"
#include "threads/malloc.h"
#include <stdbool.h>

void spt_destroy_func (struct hash_elem *e, void *aux);
unsigned hash_spt (const struct hash_elem* elem, void* aux);
bool hash_spt_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);

/*
 * Initialize supplementary page table
 */
struct hash *
spt_init (void)
{
	struct hash *spt = (struct hash *)malloc(sizeof(struct hash));
	hash_init(spt, hash_spt, hash_spt_less, NULL);
	return spt;
}

/*
 * Make new supplementary page table entry for addr 
 */
void 
allocate_spt (struct hash *spt, void *addr)
{
	struct sup_page_table_entry* spte = malloc(sizeof(struct sup_page_table_entry));
	spte -> user_vaddr = addr;
	hash_insert(spt, &(spte->elem));
}

void destroy_spt (struct hash *spt)
{
	hash_destroy(spt, spt_destroy_func);
	free(spt);
}

void spt_destroy_func (struct hash_elem *e, void *aux)
{
	free(hash_entry(e, struct sup_page_table_entry, elem));
}

unsigned hash_spt (const struct hash_elem* elem, void* aux)
{
	const void* buf_ = hash_entry(elem, struct sup_page_table_entry, elem)->user_vaddr;
	size_t size = 4;
	return hash_bytes(buf_, size);
}

bool hash_spt_less (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	return hash_entry(a, struct sup_page_table_entry, elem)->user_vaddr<hash_entry(b, struct sup_page_table_entry, elem)->user_vaddr;
}