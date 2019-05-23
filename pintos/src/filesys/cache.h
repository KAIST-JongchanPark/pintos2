#include <stdbool.h>
#include "lib/kernel/list.h"
#include "devices/disk.h"


struct cache_elem
{
	void* addr;
	disk_sector_t sector;
	bool allocated;
	bool accessed;
	bool dirty;
	struct list_elem elem;
	int index;
};

void cache_init(void);
void cache_read(struct disk *, disk_sector_t, void* , int);
void cache_read_ofs(struct disk *, disk_sector_t, void*, int ofs, int size);
void cache_write(struct disk *, disk_sector_t, void*);
void cache_write_ofs(struct disk *d, disk_sector_t sec_no, void* buffer, int ofs, int size);
struct cache_elem* cache_find(disk_sector_t);
struct list_elem* cache_empty_slot(void);
void cache_install(struct disk *, disk_sector_t);
void cache_evict(struct disk *);
void cache_write_behind(void);
