#include "inode.h"
void init_connection();
void close_connection();

int add_entry(sector_t sector, void* buf, uint_size_t size);
int get_entry(sector_t sector, char* buf, uint_size_t size);

void send_block(sector_t sector, void* buf);
void recv_block(sector_t sector, void* buf);

int flush_all();
