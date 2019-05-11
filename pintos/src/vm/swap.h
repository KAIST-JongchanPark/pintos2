#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "threads/init.h"

void swap_init (void);
bool swap_in (void *addr);
bool swap_out (void);
void read_from_disk (uint8_t *frame, size_t place, int index);
void write_to_disk (uint8_t *frame, size_t place, int index);

#endif /* vm/swap.h */
