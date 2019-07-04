#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include "inode.h"
typedef struct dir dir_t;
typedef struct dir_entry dir_entry_t;


dir_t* dir_open(inode_t* inode);
void dir_close(dir_t* dir);
int dir_create(inumber_t inumber, size_t entry_count);
int dir_remove(dir_t* dir);

int dir_read(dir_t* dir, char* file_name_buf);
int dir_add_entry(dir_t* dir, char* file_name, inumber_t inumber);
int dir_remove_entry(dir_t* dir, char* file_name);
int dir_lookup(dir_t* dir, char* file_name, inumber_t* inumber);
void dir_reset_seek(dir_t* dir);
#endif