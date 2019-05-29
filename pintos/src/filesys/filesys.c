#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct lock filesys_lock;

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);
struct dir* get_parent_dir(char* dir);
char* get_name(char* dir);


/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, file system initialization failed");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
  //thread_current()->dir = dir_open_root();
  //dir_close(thread_current()->dir);
  lock_init(&filesys_lock);
}
/*
struct dir *
open_parent_dir(char *name)
{
  //parsing
  dir_lookup(thread_current()->dir, parsingname, &inode);
  dir_open(inode);
}
*/
/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  cache_write_behind();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  lock_acquire(&filesys_lock);
  disk_sector_t inode_sector = 0;
  struct dir *dir = get_parent_dir(name);//get_parent_dir(name);
  //printf("test\n");
  //printf("name: %s\n", name);
  //printf("strlen: %d\n", strlen(name));
  if(dir==NULL||strlen(name)==0)
  {
    //printf("name: %s\n", name);
    printf("test2\n");
    return false;
  }
  //printf("strlen: %d\n", strlen(name));
  //printf("name: %s\n", name);
  char *file_name = get_name(name);
  printf("file_name: %s\n", file_name);
  bool success = dir != NULL
                  && free_map_allocate (1, &inode_sector);
  printf("success value %d\n", success);
                  success = success&& inode_create (inode_sector, initial_size, is_dir);
                  printf("success value %d\n", success);
                  success = success&& dir_add (dir, file_name, inode_sector);
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  lock_release(&filesys_lock);
  return success;
}

struct dir* get_parent_dir(char* dir)
{
  char* dir_copy = malloc(strlen(dir)+1);
  strlcpy(dir_copy, dir, strlen(dir)+1);
  struct dir* current_dir;
  char* ret_ptr;
  char* next_ptr;
  struct inode* inode;

  if(dir_copy[0]=='/')
  {
    current_dir = dir_open_root();
    ret_ptr = strtok_r(dir_copy, "/", &next_ptr);
    printf("fisrt / cut: %s", ret_ptr);
  }
  else
  {
    if(thread_current()->dir==NULL)
    {
      current_dir = dir_open_root();
    }
    else
    {
      current_dir = dir_reopen(thread_current()->dir);
    }
    
    if(current_dir == NULL)
    {
      printf("current dir is NULL\n");
    }
  }

  ret_ptr = strtok_r(dir_copy, "/", &next_ptr);
  while(ret_ptr!=NULL)
  {
    printf("string loop: %s", ret_ptr);
    if(strlen(next_ptr)==0)
    {
      printf("ret_ptr when finished: %s\n", ret_ptr);
      break;
    }
    if(!dir_lookup(current_dir, ret_ptr, &inode))
    {
      return NULL;
    }
    if(inode_is_dir(inode))
    {
      printf("ret_ptr in parent dir: %s\n", ret_ptr);
      dir_close(current_dir);
      current_dir = dir_open(inode);
    }
    else
    {
      inode_close(inode);
    }
    ret_ptr = strtok_r(NULL, "/", &next_ptr);
  }
  return current_dir;
}

char* get_name(char* dir)
{
  char* dir_copy = malloc(strlen(dir)+1);;
  strlcpy(dir_copy, dir, strlen(dir)+1);
  char* ret_ptr;
  char* next_ptr;

  ret_ptr = strtok_r(dir_copy, "/", &next_ptr);
  //printf("origin name: %s\n", dir);
  //printf("ret_ptr: %s\n", ret_ptr);
  if(strlen(next_ptr)==0)
  {
    //printf("next_ptr: %s\n", next_ptr);
  }
  while(ret_ptr!=NULL)
  {
    if(strlen(next_ptr)==0)
    {
      break;
    }
    ret_ptr = strtok_r(NULL, "/", &next_ptr);
  }
  return ret_ptr;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  lock_acquire(&filesys_lock);
  struct dir *dir = get_parent_dir(name);//get_parent_dir(name);/*dir_open_root ()*/ 
  if(dir==NULL||strlen(name)==0)
  {
    //printf("parent dir is null in fsys open\n");
    return NULL;
  }
  char *file_name = get_name(name);//parsing
  //printf("file name in fsys open: %s\n", file_name);
  struct inode *inode = NULL;

  if (dir != NULL)
  {
    //printf("dir not NULL\n");
    dir_lookup (dir, file_name, &inode);
  }
  //printf("reached\n");
  dir_close (dir);
  lock_release(&filesys_lock);
  //printf("end of filesys open\n");
  if(inode==NULL)
  {
    //printf("inode is NULL\n");
  }
  struct file* file = file_open (inode);
  if(file==NULL)
  {
    //printf("file is NULL\n");
  }
  //printf("filesys open ends\n");
  return file;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  lock_acquire(&filesys_lock);
  struct dir *dir = get_parent_dir(name);/*dir_open_root ()*/  // need to implement, open that path and return parent dir
  if(dir==NULL||strlen(name)==0)
    return false;
  char *file_name = get_name(name);//parsing
  /*
  if(is_dir)
  {
    struct inode* inode;
    dir_lookup(dir, file_name, &inode);
    struct dir* target_dir = dir_open(inode);
    if(!dir_isempty(target_dir))//만들어야함. 
    {
      return false;
    }
  }
  */
  bool success = dir != NULL && dir_remove (dir, file_name);
  dir_close (dir); 
  lock_release(&filesys_lock);
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
