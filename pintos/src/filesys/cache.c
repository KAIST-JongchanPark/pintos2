#include "filesys/cache.h"
#include "lib/kernel/list.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "devices/disk.h"
#include "lib/string.h"
#include "filesys/filesys.h"
#include <stdio.h>

struct list cache_list;
struct lock cache_lock;

int evict_counter;

//Initialize cache_list at init.c
void cache_init(void)
{
	//init the list
	printf("cache_init started\n");
	list_init(&cache_list);
	lock_init(&cache_lock);
	printf("1\n");
	int i;
	struct cache_elem *celem;
	void *upage;
	evict_counter = 0;

	//push elements into the list
	for(i=0;i<64;i++)
	{
		printf("2\n");
		if(!i%8)
		{
			upage = palloc_get_page(PAL_USER|PAL_ZERO);
		}
		else
		{
			upage+=512;
		}
		printf("3\n");
		celem = malloc(sizeof(struct cache_elem)); // *빼야될듯?
		celem->addr = upage;
		celem->allocated = false;
		celem->index = i;
		list_push_back(&cache_list, &celem->elem);
	}
	printf("cache_init finished\n");

}

//동기화 하기 위해서 cache_r/w에 lock 걸어놓고, 나머지 함수는 그 안에서만 부르면 될듯?

//disk_read 대신 cache 에서 읽는 함수
void cache_read(struct disk *d, disk_sector_t sec_no, void *buffer)
{
	lock_acquire(&cache_lock);
	struct cache_elem* celem = cache_find(sec_no);
	if(!celem)
	{
		cache_install(d, sec_no);
		celem = cache_find(sec_no);
	}

	memcpy(buffer, celem->addr, 512);
	celem->accessed = true;
	lock_release(&cache_lock);
}

//inode 참고. 
void cache_read_ofs(struct disk *d, disk_sector_t sec_no, void* buffer, int ofs, int size)
{
	lock_acquire(&cache_lock);
	struct cache_elem* celem = cache_find(sec_no);
	if(!celem)
	{
		cache_install(d, sec_no);
		celem = cache_find(sec_no);
	}
	memcpy(buffer, celem->addr+ofs, size);
	celem->accessed = true;
	lock_release(&cache_lock);
}

//disk_write 대신. 
void cache_write(struct disk *d, disk_sector_t sec_no, void *buffer)
{
	lock_acquire(&cache_lock);
	struct cache_elem* celem = cache_find(sec_no);
	if(!celem)
	{
		cache_install(d, sec_no);
		celem = cache_find(sec_no);
	}
	memcpy(celem->addr, buffer, 512);
	celem->accessed = true;
	celem->dirty = true;
	lock_release(&cache_lock);
}

//inode 참고, _ofs 함수들 맞게 짠건지 확실하지 않음..(그냥 합쳐도 될듯한테 틀렸을 수도 있으니까 남겨놓음)
void cache_write_ofs(struct disk *d, disk_sector_t sec_no, void* buffer, int ofs, int size)
{
	lock_acquire(&cache_lock);
	struct cache_elem* celem = cache_find(sec_no);
	if(!celem)
	{
		cache_install(d, sec_no);
		celem = cache_find(sec_no);
	}
	memcpy(celem->addr+ofs, buffer, size);
	celem->accessed = true;
	celem->dirty = true;
	lock_release(&cache_lock);
}

//cache 에 주어진 sector 번호가 있는지 확인하는 함수, 없으면 NULL
struct cache_elem* cache_find(disk_sector_t sec_no)
{
	struct list_elem *e;
	struct cache_elem* celem;
	for(e = list_begin(&cache_list) ; e != list_end(&cache_list) ; e = list_next(e))
	{
		celem = list_entry(e, struct cache_elem, elem);
		if(celem->sector == sec_no)
			return celem;
	}
	return NULL;
}

//비어있는 공간 찾기, 꽉 차있으면 NULL
struct list_elem* cache_empty_slot(void)
{
	struct list_elem *e;
	struct cache_elem* celem;
	for(e = list_begin(&cache_list) ; e != list_end(&cache_list) ; e = list_next(e))
	{
		celem = list_entry(e, struct cache_elem, elem);
		if(celem->allocated == false)
			return e;
	}
	return NULL;
}

//만약 공간이 있다면 그냥 거기에 초기화하기 , 아니면 캐시중에 하나 evict 하고 주어진 내용으로 캐시에 쓰
void cache_install(struct disk *d, disk_sector_t sec_no)
{
	struct cache_elem* celem;
	struct list_elem* cache_slot = cache_empty_slot();
	if(!cache_slot)
	{
		cache_evict(d);
		cache_slot = cache_empty_slot();
		
	}
	//there should be an empty space, initialize it. 
	celem = list_entry(cache_slot, struct cache_elem, elem);
	celem->allocated = true;
	celem->accessed = false;
	celem->dirty = false;
	celem->sector = sec_no;
	disk_read(d, sec_no, celem->addr); 

}

//캐시중에 evict할 entry를 찾아서 evict 하고(disk에 다시 쓰고) return 하기. 우선 FIFO 로 만들어놓고 나중에 
void cache_evict(struct disk *d)
{
	struct list_elem *e;
	struct cache_elem* celem;
	for(e = list_begin(&cache_list) ; e != list_end(&cache_list) ; e = list_next(e))
	{
		celem = list_entry(e, struct cache_elem, elem);
		if(celem->index == evict_counter)
			break;
	}
	celem->allocated = false;
	disk_write(d, celem->sector, celem->addr);
	memset(celem->addr, 0, 512);
	evict_counter = evict_counter==63? 0 : evict_counter+1;

}

//write behind thread 모르겠어서 그냥 함수로만 만들어놓음. 
void cache_write_behind(void)
{
	//lock_acquire(&cache_lock);
	//printf("cache_write_behind started\n");
	
	struct list_elem *e;
	struct cache_elem* celem;
	for(e = list_begin(&cache_list) ; e != list_end(&cache_list) ; e = list_next(e))
	{
		celem = list_entry(e, struct cache_elem, elem);
		celem->allocated = false;
		if(celem->dirty)
		{
			// celem->dirty = false; //이것도 해야될듯?
			disk_write(filesys_disk, celem->sector, celem->addr); // sec_no가 아니라 celem->sector?
		}
		memset(celem->addr, 0, 512);
	}
	evict_counter = 0;
	exit(0);
	//printf("cache_write_behind finished\n");

	//lock_release(&cache_lock);
}
//read ahead 하기 이거 어떻게 하는지 잘 모르겠음. (뭘 하라고 하는건지.. )

// periodical하게 disk에 write 해주기 100 마다?

//eviction 알고리즘도 만들어야 함. 

