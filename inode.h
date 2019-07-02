#ifndef _INODE_H
#define _INODE_H

#include "list.h"
#include <stdlib.h>

#define BLOCK_SIZE 1024
#define INODE_PAD_SIZE 1004

#define FREE_MAP_BLOCKx 0
#define ROOT_DIR_BLOCK 1


typedef unsigned long long inumber_t;
typedef size_t block_t;

struct disk_inode{
  inumber_t inumber; //8
  size_t length; //8
  int mode; //4 
  char unused[INODE_PAD_SIZE];
};
typedef struct disk_inode disk_inode;

struct inode{
  inumber_t inumber;
  size_t open_count;
  struct list_elem elem;
  disk_inode d_inode;
};
typedef struct inode inode_t;


void init_inode();
int inode_create(inumber_t inumber, size_t size, int mode);
inode_t* inode_open(inumber_t inumber);
void inode_close(inumber_t inumber);
int inode_write(inode_t* inode, void* buf, size_t offset, size_t size);
int inode_read(inode_t* inode, void* buf, size_t offset, size_t size);


size_t bytes_to_nblock(size_t bytes);
block_t bytes_to_index(size_t bytes);
inumber_t block_to_inumber(inumber_t inumber, block_t block);

#endif