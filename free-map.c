#include "free-map.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "inode.h"
inumber_t succesive_inode_inumber;
inode_t* freemap_inode;
char empty_buf[BLOCK_SIZE];

/*  this implementation will suffice for now
 *  implementation will be extended
 *  to support filling gaps after
 *  max block index is reached 
 */

struct free_map {

};


int init_free_map(){
    // 0 for free-map file
    // 1 for root dir
    freemap_inode = inode_open(FREE_MAP_INUMBER);
    int status = 0;
    if(freemap_inode == NULL)
        return -1;
    char buf[BLOCK_SIZE];
    if (inode_read(freemap_inode, buf, 0, BLOCK_SIZE) < 0){
        inode_close(freemap_inode);
        return -1;
    }
    succesive_inode_inumber = atoi(buf);
    printf("%llu***********\n", succesive_inode_inumber);
    if(succesive_inode_inumber < 2)
        succesive_inode_inumber = 2;
    printf("%llu***********corrected\n", succesive_inode_inumber);
    
    memset(empty_buf, 0, BLOCK_SIZE);
    return 0;
}

int open_freemap_inode(){
    freemap_inode = inode_open(FREE_MAP_INUMBER);
    if(freemap_inode == NULL)
        return -1;
    return 0;
}

int create_freemap_inode(){
    if(inode_create(FREE_MAP_INUMBER, 0, __S_IFREG) < 0)
        return -1;
}

inumber_t alloc_inumber(){
    inumber_t res = succesive_inode_inumber << 4*8;
    succesive_inode_inumber++;
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    sprintf(buf, "%llu", succesive_inode_inumber);
    inode_write(freemap_inode, buf, 0, BLOCK_SIZE);
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
        if(status < 0)
            return -1;
    }
    return 0;
}

void free_block(inumber_t block){

}

void free_map_finish(){
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    sprintf(buf, "%llu", succesive_inode_inumber);
    inode_write(freemap_inode, buf, 0, BLOCK_SIZE);
    inode_close(freemap_inode);
}
