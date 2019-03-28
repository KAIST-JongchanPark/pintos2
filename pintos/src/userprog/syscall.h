#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void* is_valid_ptr(void*);
void exit_with_status (int);
int write (int, const void *, unsigned);
void syscall_init (void);


#endif /* userprog/syscall.h */
