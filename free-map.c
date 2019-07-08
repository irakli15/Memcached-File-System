#include "free-map.h"
#include <string.h>
#include <stdio.h>
// #include "inode.h"
inumber_t succesive_inode_inumber;

char empty_buf[BLOCK_SIZE];

/*  this implementation will suffice for now
 *  implementation will be extended
 *  to support filling gaps after
 *  max block index is reached 
 */

struct free_map {

};


void init_free_map(){
    // 0 for free-map file
    // 1 for root dir
    succesive_inode_inumber = 2;
    memset(empty_buf, 0, BLOCK_SIZE);
}

inumber_t alloc_inumber(){
    inumber_t res = succesive_inode_inumber << 4*8;
    succesive_inode_inumber++;
    return res;
}

// must call this before updating length in inode
int alloc_blocks(disk_inode* d_inode, size_t count){
    //first 4 bytes - inode number, second 4 bytes block number
    block_t start_block;

    if(d_inode->length == 0)
        start_block = 1;
    else
        start_block = bytes_to_nblock(d_inode->length) + 1;
    // printf("start %lu\n", start_block);
    //next 2 lines for the indice to start from 0
    size_t i = 0;
    count += i;
    int status = 0;
    for(; i < count; i++){
        // printf("%llu\n", block_to_inumber(d_inode->inumber, i + start_block));
        status = add_block(block_to_inumber(d_inode->inumber, i + start_block), empty_buf);
        if(status != 0)
            return -1;
    }
    return 0;
}

void free_block(inumber_t block){

}
