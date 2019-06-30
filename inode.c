#include "inode.h"
#include "free-map.h"
#include <string.h>
#include <stdio.h>

struct list opened_inodes;

void init_inode(){
    // check for inode structure in memcached
    // if not found, create new:
    // this should be done with mkdir.
    // inode_create_specific(ROOT_DIR_BLOCK, 10, 0); // need to get S_IFDIR

    list_init(&opened_inodes);
}

uint_size_t bytes_to_nblock(uint_size_t bytes){
    block_t res = bytes/BLOCK_SIZE;
    return bytes > res * BLOCK_SIZE ? res + 1 : res;
}

block_t bytes_to_index(uint_size_t bytes){
    return bytes/BLOCK_SIZE;
}

inumber_t block_to_inumber(inode_t* inode, block_t block){
    return (inode->inumber << 4) | block;
}


int inode_create(inumber_t inumber, uint_size_t size, int mode){
    uint_size_t block_count = bytes_to_nblock(size);
    disk_inode d_inode; 
    d_inode.inumber = inumber;
    int status = alloc_blocks(&d_inode, block_count);
    if(status == -1)
        return status;

    d_inode.length = size;

    status = add_block(inumber, &d_inode);

    return inumber;
}

inode_t* lookup_inode(inumber_t inumber){
    struct list_elem *e;
    for (e = list_begin (&opened_inodes); e != list_end (&opened_inodes);
           e = list_next (e)){
          inode_t *i = list_entry (e, inode_t, elem);
          if(i->inumber == inumber)
            return i;
        }
    return NULL;
}

inode_t* inode_open(inumber_t inumber){
  inode_t* inode = lookup_inode(inumber);
  if(inode != NULL){
      inode->open_count++;
      return inode;
  }

  inode = malloc(sizeof(inode_t));
  int status = get_block(inumber, &inode->d_inode);
  if(status == -1){
      printf("getting inumber:%llu block failed\n", inumber);
      free(inode);
      return NULL;
  }
  inode->inumber = inode->d_inode.inumber;
  inode->open_count = 1;
  list_push_front(&opened_inodes, &inode->elem);
  return inode;
}

void inode_close(inumber_t inumber){
    inode_t* inode = lookup_inode(inumber);
    if(inode == NULL)
        return;

    inode->open_count--;
    if(inode->open_count == 0){
        list_remove(&inode->elem);
        free(inode);
    }
}

int inode_write(inode_t* inode, void* buf, uint_size_t offset, uint_size_t size){
  return 0;
}

int inode_read(inode_t* inode, void* buf, uint_size_t offset, uint_size_t size){
    block_t block_index;
    uint_size_t off_in_block;
    uint_size_t left_in_block;
    uint_size_t read_size ;
    // left_in_block = 
    uint_size_t read = 0;
    uint_size_t left_in_inode;

    char temp_buf[BLOCK_SIZE];
    int status = 0;
    //debug
    printf("starting offset: %u\n", offset);
    printf("total size: %u\n", size);

    while(size > 0){
        // if reading goes past inode length
        if(offset >= inode->d_inode.length){
            memset(buf + read, 0, size);
            printf("get out\n");
            return 0;
        }


        block_index = bytes_to_index(offset);
        off_in_block = offset % BLOCK_SIZE;
        left_in_block = BLOCK_SIZE - off_in_block;

        // read size must be a mininum between
        // what is left to read, what is remaining in 
        // current block and what is remaining in inode
        read_size = left_in_block > size ? size : left_in_block;
        left_in_inode = inode->d_inode.length - offset;
        read_size = read_size > left_in_inode ? left_in_inode : read_size;
        
     

        //debug
        // printf("block_index: %u\n", block_index);
        // printf("off_in_block: %u\n", off_in_block);
        // printf("left_in_block: %u\n", left_in_block);
        // printf("left_in_inode: %u\n", left_in_inode);
        // printf("left size: %u\n", size);
        // printf("read_size: %u\n", read_size);
        // printf("offset: %u\n", offset);
        // printf("\n");

        status = get_block(block_to_inumber(inode, block_index), temp_buf);
        if(status != 0){
            printf("error while reading %llu inumber\n", inode->inumber);
            return -1;
        }

        memcpy(buf + read, temp_buf + off_in_block, read_size);

        size -= read_size;
        read += read_size;
        offset += read_size;    
    }
  return 0;
}

int main(){
    char buff[1500];
    inode_t inode;
    inode.d_inode.length = 1024+399+1500;
    printf("inode length: %u \n", inode.d_inode.length);
    inode_read(&inode, buff, 1024 + 400, 1500);
    return 0;
}
