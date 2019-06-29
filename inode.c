#include "inode.h"
#include <math.h>


void init_inode(){
    // check for inode structure in memcached
    // if not found, create new:

    inode_create(FREE_MAP_SECTOR, 10, 0); // latter two arguments are arbitrary for now
    inode_create(ROOT_DIR_SECTOR, 10, 0); // need to get S_IFDIR
}

int bytes_to_nsector(uint_size_t bytes){
    int res = bytes/SECTOR_SIZE;
    return bytes > res*SECTOR_SIZE ? res + 1 : res;
}

int inode_create(sector_t sector, uint_size_t size, int mode){
    return 0;
}