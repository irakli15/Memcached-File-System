#ifndef _INODE_H
#define _INODE_H

#include "list.h"
#include <stdlib.h>

#define BLOCK_SIZE 4096
#define LEFT_OVER (BLOCK_SIZE - 36)

#define XATTR_COUNT (LEFT_OVER/172)
#define INODE_PAD_SIZE (LEFT_OVER - (XATTR_COUNT*172))

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
struct xattr {
  char key[43];
  char value[128];
  char size; //is it in use?
};
typedef struct xattr xattr_t;

struct disk_inode{
  inumber_t inumber; //8
  size_t length; //8
  int mode; //4 
  size_t nlink; // 8
  xattr_t xattrs[XATTR_COUNT];
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
void inode_chmod(inode_t* inode, int mode);


int increase_nlink(inode_t* inode);
int decrease_nlink(inode_t* inode);

int inode_set_xattr(inode_t* inode, const char* key, const char* value, size_t size);
int inode_remove_xattr(inode_t* inode, const char* key);
xattr_t* inode_get_xattr(inode_t* inode, const char* key);
size_t inode_xattr_list_len(inode_t* inode);
void inode_xattr_list(inode_t* inode, char* list);
#endif