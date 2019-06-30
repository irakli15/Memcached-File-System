#include "free-map.h"
#include <string.h>
#include <stdio.h>
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
    succesive_inode_inumber++;
}

// must call this before updating length in inode
// returns index where the new blocks start
int alloc_blocks(disk_inode* inode, uint_size_t count){
    //first 4 bytes - inode number, second 4 bytes block number

    block_t start_block;

    if(inode->length == 0)
        start_block = 0;
    else
        start_block = bytes_to_nblock(inode->length) + 1;
        
    inumber_t index = (inode->inumber << 4) | start_block;
    //next 2 lines for the indice to start from 0
    uint_size_t i = inode->length == 0 ? 0 : 1;
    count += i;
    int status = 0;
    for(; i < count; i++){
        status = add_block(index + i, empty_buf);
        if(status != 0)
            return -1;
        // printf("sent %llu\n", index+i);
    }
    return start_block;
}

void free_block(inumber_t block){

}
