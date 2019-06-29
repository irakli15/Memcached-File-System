#ifndef _FREE_MAP_H
#define _FREE_MAP_H

#include "memcached_client.h"
#include "inode.h"


//get free map file from memcached
//and populate bitmap
void init_free_map();

sector_t alloc_sector();
void free_sector(sector_t sector);

#endif