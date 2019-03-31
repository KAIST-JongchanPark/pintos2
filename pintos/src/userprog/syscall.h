#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef uint32_t bool;

void* is_valid_ptr(void*);
void exit_with_status (int);
void syscall_init (void);


int write (int, const void *, unsigned);
void halt (void) ;
void exit (int) ;
//pid_t exec (const char *);
int wait (tid_t);
bool create (const char *, unsigned);
bool remove (const char *);
int open (const char *);
int filesize (int);
int read (int, void *, unsigned );
int write (int, const void *, unsigned );
void seek (int, unsigned );
unsigned tell (int);
void close (int);



#endif /* userprog/syscall.h */


