#include "inode.h"
#include "free-map.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

// #define DEBUG_READ
//  #define DEBUG_WRITE

struct list opened_inodes;

size_t ilen(inode_t* inode);

void init_inode(){
    // check for inode structure in memcached
    // if not found, create new:
    // this should be done with mkdir.
    // inode_create_specific(ROOT_DIR_BLOCK, 10, 0); // need to get S_IFDIR

    list_init(&opened_inodes);
    assert(sizeof(disk_inode) == BLOCK_SIZE);
}

size_t bytes_to_nblock(size_t bytes){
    block_t res = bytes/BLOCK_SIZE;
    return bytes > res * BLOCK_SIZE ? res + 1 : res;
}

block_t bytes_to_index(size_t bytes){
    return bytes/BLOCK_SIZE + 1;
}

inumber_t block_to_inumber(inumber_t inumber, block_t block){
    return inumber | block;
}


int inode_create(inumber_t inumber, size_t size, int mode){
    size_t block_count = bytes_to_nblock(size);
    disk_inode d_inode; 
    d_inode.inumber = inumber;
    d_inode.length = 0;
    int status = alloc_blocks(&d_inode, block_count);
    if(status != 0)
        return status;

    d_inode.length = size;

    status = add_block(inumber, &d_inode);
    if (status != 0){
        return status;
    }

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

void grow_inode(inode_t* inode, size_t final_size){
    size_t block_count = bytes_to_nblock( final_size- ilen(inode));
    // exit(0);
    alloc_blocks(&inode->d_inode, block_count);
}

int inode_write(inode_t* inode, void* buf, size_t offset, size_t size){
    block_t block_index;
    size_t off_in_block;
    size_t left_in_block;
    size_t write_size ;
    size_t written = 0;
    size_t left_in_inode;

    char temp_buf[BLOCK_SIZE];
    int status = 0;
    //debug
    #ifdef DEBUG_WRITE
        printf("write starting offset: %lu\n", offset);
        printf("write total size: %lu\n", size);
    #endif

    while(size > 0){
        block_index = bytes_to_index(offset);
        off_in_block = offset % BLOCK_SIZE;
        left_in_block = BLOCK_SIZE - off_in_block;

        #ifdef DEBUG_WRITE
            printf("offset: %lu\n", offset);
        #endif


        // write size must be a mininum between
        // what is left to write and what is remaining in 
        // current block
        write_size = left_in_block > size ? size : left_in_block;

              //debug
        #ifdef DEBUG_WRITE
            printf("block_index: %lu\n", block_index);
            printf("off_in_block: %lu\n", off_in_block);
            printf("left_in_block: %lu\n", left_in_block);
            printf("left size: %lu\n", size);
            printf("write_size: %lu\n", write_size);
        #endif
        
        if(offset + write_size >= ilen(inode)){
            grow_inode(inode, offset + write_size);
            #ifdef DEBUG_WRITE
                // printf("adding %lu blocks\n", block_count);
            #endif
            if (status != 0){
                printf("error while adding new block\n");
                return -1;
            }
        }


        if (off_in_block == 0 && write_size == BLOCK_SIZE){
            status = 0;
            memset(temp_buf, 0, BLOCK_SIZE);
        } else {
            status = get_block(block_to_inumber(inode->inumber, block_index), temp_buf);
        }


        if(status != 0){
            printf("error while writing %llu inumber\n", block_to_inumber(inode->inumber, block_index));
            return -1;
        }

        memcpy(temp_buf + off_in_block, buf + written, write_size);
        update_block(block_to_inumber(inode->inumber, block_index), temp_buf);


        inode->d_inode.length += (write_size - ilen(inode) + offset );
        #ifdef DEBUG_WRITE
            printf("new size:%lu\n", ilen(inode));
            printf("\n");
        #endif
        size -= write_size;
        written += write_size;
        offset += write_size;
    }
    status = update_block(inode->inumber, &inode->d_inode);

  return status;
}

int inode_read(inode_t* inode, void* buf, size_t offset, size_t size){
    block_t block_index;
    size_t off_in_block;
    size_t left_in_block;
    size_t read_size ;
    size_t read = 0;
    size_t left_in_inode;

    char temp_buf[BLOCK_SIZE];
    int status = 0;
    //debug
    #ifdef DEBUG_READ
        printf("read starting offset: %lu\n", offset);
        printf("inode length: %lu\n", inode->d_inode.length);

        printf("read total size: %lu\n", size);
    #endif
    while(size > 0){
        // if reading goes past inode length
        if(offset >= inode->d_inode.length){
            memset(buf + read, 0, size);

            #ifdef DEBUG_READ
                printf("offset %lu\n", offset);
                printf("inode length: %lu\n", inode->d_inode.length);            
                printf("get out\n");
            #endif

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
        #ifdef DEBUG_READ
            printf("block_index: %lu\n", block_index);
            printf("off_in_block: %lu\n", off_in_block);
            printf("left_in_block: %lu\n", left_in_block);
            printf("left_in_inode: %lu\n", left_in_inode);
            printf("left size: %lu\n", size);
            printf("read_size: %lu\n", read_size);
            printf("offset: %lu\n", offset);
            printf("\n");
        #endif

        status = get_block(block_to_inumber(inode->inumber, block_index), temp_buf);
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

void inode_delete(inode_t* inode){
    inumber_t max_index = block_to_inumber(inode->inumber, bytes_to_index(ilen(inode)));
    int status = 0;
    for(inumber_t i = inode->inumber; i <= max_index; i++){
        status = remove_block(i);
        if(status != 0){
            printf("error while removing %llu block\n", i);
        }
    }
}

size_t ilen(inode_t* inode){
    return inode->d_inode.length;
}

/*
 * creates inode, writes once forcing to
 * create a new block
 */
void single_write(){
    char buff[1024];
    int debug = 0;
    inumber_t inumber = 5 << 4;
    inode_create(inumber, 1024, 0);
    inode_t* inode = inode_open(inumber);
    if(inode == NULL){
        printf("null\n");
        flush_all();
        return;
    }
    if(debug)
    printf("inumber: %llu, length: %lu\n", inode->inumber, inode->d_inode.length);
    
    char* hello_buff = "hello";
    inode_write(inode, hello_buff, 1024, strlen(hello_buff));

    inode_close(inumber);
    inode = inode_open(inumber);
    if(debug)
    printf("inumber: %llu, length: %lu\n", inode->inumber, inode->d_inode.length);


    char read_back[10];
    inode_read(inode, read_back, 1024, 10);//strlen(hello_buff));
    if(debug)
    printf("%s\n", read_back);

    if(strcmp(read_back, hello_buff) == 0){
        printf("single_write passed\n");
    } else {
        printf("single_write failed\n");
    }
    flush_all();
}

/*
 * write once and write will stretch
 * over two blocks, then read it back
 */
void write_stretch_and_big_seek(){
    int debug=0;
    char buff[1024];
    inumber_t inumber = 5 << 4;
    inode_create(inumber, 1024, 0);
    inode_t* inode = inode_open(inumber);
    if(inode == NULL){
        printf("null\n");
        flush_all();
        return;
    }
    if(debug)
    printf("inumber: %llu, length: %lu\n", inode->inumber, inode->d_inode.length);
    
    char* hello_buff = "hello";
    inode_write(inode, hello_buff, 1022, strlen(hello_buff));

    inode_close(inumber);
    inode = inode_open(inumber);
    if(debug)
    printf("inumber: %llu, length: %lu\n", inode->inumber, inode->d_inode.length);


    char read_back[10];
    //read more than the length of written message
    inode_read(inode, read_back, 1022, 10);//strlen(hello_buff));
    if(debug)
    printf("%s\n", read_back);

    if(strcmp(read_back, hello_buff) == 0){
        printf("single_write_stretch passed\n");
    } else {
        printf("single_write_stretch failed\n");
    }

    char* msg = "message";
    inode_write(inode, msg, 4096, strlen(msg));
    char read_back2[10];

    inode_read(inode, read_back2, 4096, 10);
    if(debug)
    printf("%s\n", read_back2);

    if(strcmp(read_back, hello_buff) == 0){
        printf("single_write_big_seek passed\n");
    } else {
        printf("single_write_big_seek failed\n");
    }

    inode_delete(inode);

    flush_all();
}

int main(){
    init_connection();
    init_inode();
    flush_all();
    single_write();
    write_stretch_and_big_seek();    

    
    return 0;
}

