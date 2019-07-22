#ifndef _FREE_MAP_H
#define _FREE_MAP_H

#include "memcached_client.h"
#include "inode.h"


//get free map file from memcached
//and populate bitmap
int init_free_map();
void free_map_finish();
int open_freemap_inode();
int create_freemap_inode();
inumber_t alloc_inumber();
int alloc_blocks(disk_inode* d_inode, size_t count);
void free_block(inumber_t block);

#endif