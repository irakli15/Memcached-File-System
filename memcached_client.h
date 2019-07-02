#ifndef _MEMCACHED_CLIENT_H
#define _MEMCACHED_CLIENT_H

#include "inode.h"
void init_connection();
void close_connection();

int add_entry(inumber_t block, void* buf, size_t size);
int get_entry(inumber_t block, char* buf, size_t size);
int update_entry(inumber_t block, void* buf, size_t size);
int remove_entry(inumber_t block);

int add_block(inumber_t block, void* buf);
int get_block(inumber_t block, void* buf);
int update_block(inumber_t block, void* buf);
int remove_block(inumber_t block);

int flush_all();

#endif
