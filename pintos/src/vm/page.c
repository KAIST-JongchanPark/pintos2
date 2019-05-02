#include "vm/page.h"
#include "lib/kernel/hash.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "userprog/process.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "vm/frame.h"
#include <stdbool.h>

void spt_destroy_func (struct hash_elem *e, void *aux);
unsigned hash_spt (const struct hash_elem* elem, void* aux);
bool hash_spt_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);

/*
 * Initialize supplementary page table
 */
struct hash *spt_init (void)
{
	struct hash *spt = (struct hash *)malloc(sizeof(struct hash));
	hash_init(spt, hash_spt, hash_spt_less, NULL);
	return spt;
}

/*
 * Make new supplementary page table entry for addr 
 */
void allocate_spt (struct hash *spt, void *addr)
{
	struct sup_page_table_entry* spte = malloc(sizeof(struct sup_page_table_entry));
	spte -> page = addr;
	hash_insert(spt, &(spte->elem));
}

void free_spt (struct sup_page_table_entry* spte)
{
	struct hash *spt = thread_currnet()->spt;
	struct hash_elem elem = spte->elem;
	remove(spt, &elem);
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

bool allocate_and_init_to_zero(void* addr)
{
  uint8_t *kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  
  if (kpage == NULL)
  {
	return false;
  }
  allocate_frame((void *)kpage);
  
  if (!install_page (addr, kpage, TRUE)) 
  {
	  palloc_free_page (kpage);
	  //here
	  free_frame((void *)kpage);
	  free_spt(spt_get_page(addr));
	  //
	  return false; 
  }
  return true;
}

bool allocate_using_spt(void* addr)
{
	struct sup_page_table_entry *spte = spt_get_page(addr);
	struct file* file = spte->file;
	off_t ofs = spte->ofs;
	uint32_t page_read_bytes = spte->read_bytes;
	uint32_t page_zero_bytes = PGSIZE-spte->read_bytes;
	
	uint8_t *kpage = palloc_get_page (PAL_USER);
	
	  if (kpage == NULL)
	  {
		return false;
	  }
	
	  allocate_frame((void *)kpage);
	  
	  /* Load this page. */
	  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
		{
		  palloc_free_page (kpage);
		  //here
		  free_frame((void *)kpage);
		  free_spt(spte);
		  //
		  return false; 
		}
	  memset (kpage + page_read_bytes, 0, page_zero_bytes);

	  /* Add the page to the process's address space. */
	  bool result = pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable);
		  
	  if (!result) 
		{
		  palloc_free_page (kpage);
		  //here
		  free_frame((void *)kpage);
		  free_spt(spte);
		  //
		  return false; 
		}

	return true;
}

bool lookup_spt(void* addr)
{
	struct sup_page_table_entry* spt = malloc(sizeof(struct sup_page_table_entry));
	//spt->page = lookup_page(addr);
	spt -> page = addr & PTE_ADDR;
	return hash_find(thread_currnet()->spt, &(spt->elem))!=NULL;
}

struct sup_page_table_entry *spt_get_page(void *addr)
{
	struct sup_page_table_entry* spt = malloc(sizeof(struct sup_page_table_entry));
	spt->page = lookup_page(addr);
	struct hash_elem *e;
	e = hash_find(thread_currnet()->spt, &(spt->elem));
	return e != NULL ? hash_entry (e, struct sup_page_table_entry, elem) : NULL;
}