#ifndef _INODE_H
#define _INODE_H

#include "list.h"
#include <stdlib.h>

#define BLOCK_SIZE 4096
#define INODE_PAD_SIZE BLOCK_SIZE - 20

#define FREE_MAP_INUMBER 0
#define ROOT_DIR_INUMBER 4294967296 // 1 << 4*8


typedef unsigned long long inumber_t;
typedef size_t block_t;

/*
   * assuming neither block or inode number
   * will go over 4 bytes.
   * block count of 4 bytes translates into 
   * file size well above 1GB.
   * precisely: 2^32 kb, which is 2^12 GB
   * 
   * max possible empty inode count
   * in 2gb with 1kb of block size is:
   * 2048*1024*1024/1024 = 2048*1024 = 2^21
   * 
   * max inode count with 4byte inumber is:
   * 2^32
   * 
*/ 

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
void inode_close(inode_t* inode);
int inode_write(inode_t* inode, void* buf, size_t offset, size_t size);
int inode_read(inode_t* inode, void* buf, size_t offset, size_t size);
int inode_delete(inode_t* inode);
size_t ilen(inode_t* inode);

size_t bytes_to_nblock(size_t bytes);
block_t bytes_to_index(size_t bytes);
inumber_t block_to_inumber(inumber_t inumber, block_t block);

#endif