#include "vm/swap.h"
#include "devices/disk.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include <stdbool.h>

/* The swap device */
static struct disk *swap_device;

/* Tracks in-use and free swap slots */
static struct bitmap *swap_table;

/* Protects swap_table */
static struct lock swap_lock;

/* 
 * Initialize swap_device, swap_table, and swap_lock.
 */
void 
swap_init (void)
{
  swap_device = disk_get(1,1);
  swap_table = bitmap_create(8*1024);
  lock_init(&swap_lock);
}


bool 
swap_in (void *addr) // when page_fault but already evicted addr called.
{
	/*
	 * Reclaim a frame from swap device.
	 * 1. Check that the page has been already evicted. 
	 */
	struct sup_page_table_entry *spte = spt_get_page(addr);
	if(!spte->swapped)
	{
		PANIC("Not a swapped page");
	}

	 /* 2. You will want to evict an already existing frame
	 * to make space to read from the disk to cache. 
	 */
	uint8_t *kpage = palloc_get_page (PAL_USER);
    uint8_t *upage = (uint8_t *)((uint32_t)addr & ~PGMASK);
	if(kpage==NULL)
	{
		bool result = swap_out();
	}
	kpage = palloc_get_page (PAL_USER);

	 /* 3. Re-link the new frame with the corresponding supplementary
	 * page table entry. 
	 */
	spte->swapped = false;
	alloc_frame(kpage, addr);
	pagedir_set_page(thread_current()->pagedir, upage, kpage, spte->writable);



	 /* 4. Do NOT create a new supplementray page table entry. Use the 
	 * already existing one. 
	 */


	 /* 5. Use helper function read_from_disk in order to read the contents
	 * of the disk into the frame. 
	 */ 
	int i = 0;
	for(i=0; i<8; i++)
	{
		read_from_disk(kpage, spte->swapped_place+i);
	}

}


bool
swap_out (void) // when palloc is null, page full.
{
	/* 
	 * Evict a frame to swap device. 
	 * 1. Choose the frame you want to evict. 
	 * (Ex. Least Recently Used policy -> Compare the timestamps when each 
	 * frame is last accessed)
	 */
	struct frame_table_entry* fte = find_frame_to_evict();
	void* upage = fte->upage;
	void* kpage = fte->kpage;
	struct sup_page_table_entry * spte = spt_get_page(upage);
	 /* 2. Evict the frame. Unlink the frame from the supplementray page table entry
	 * Remove the frame from the frame table after freeing the frame with
	 * pagedir_clear_page. 
	 */

	uint32_t *pd = thread_current()->pagedir;
	pagedir_clear_page(pd, upage);
	//fte -> upage = NULL;
	spte -> swapped = true;

	 /* 3. Do NOT delete the supplementary page table entry. The process
	 * should have the illusion that they still have the page allocated to
	 * them. 
	 */

	 /* 4. Find a free block to write you data. Use swap table to get track
	 * of in-use and free swap slots.
	 */
	bool dirty_bit = fte->dirty;
	dirty_bit = dirty_bit||pagedir_is_dirty(pd, upage)||pagedir_is_dirty(pd, kpage);
	if(!dirty_bit)
	{
		free_frame(upage);
		pagedir_set_dirty(pd, upage, false);
		pagedir_set_dirty(pd, kpage, false);
		palloc_free_page(kpage);
		return true;
	}
	size_t place = bitmap_scan(swap_table, 0, 8, false);
	if(place==BITMAP_ERROR)
	{
		PANIC("swap slots are fully used.");
	}
	int i=0;
	for(i=0; i<8; i++)
	{
		write_to_disk(kpage, place+i);
	}
	bitmap_set_multiple(swap_table, place, 8, true);
	
	free_frame(upage);
	pagedir_set_dirty(pd, upage, false);
	pagedir_set_dirty(pd, kpage, false);
	palloc_free_page(kpage); // add
	spte->swapped_place = place;
	return true;


}

/* 
 * Read data from swap device to frame. 
 * Look at device/disk.c
 */
void read_from_disk (uint8_t *frame, int index)
{
	disk_read(swap_device, index, frame+index*512);

}

/* Write data to swap device from frame */
void write_to_disk (uint8_t *frame, int index)
{
	disk_write(swap_device, index*512, frame+index*512);

}

