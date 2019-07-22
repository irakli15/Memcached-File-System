#include "inode.h"
#include "free-map.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

// #define DEBUG_READ
//  #define DEBUG_WRITE

struct list opened_inodes;



void init_inode(){
    // check for inode structure in memcached
    // if not found, create new:
    // this should be done with mkdir.
    // inode_create_specific(ROOT_DIR_BLOCK, 10, 0); // need to get S_IFDIR

    list_init(&opened_inodes);
    if(sizeof(disk_inode) != BLOCK_SIZE){
        printf("sizeof: %lu\n", sizeof(disk_inode));
    }
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
    d_inode.mode = mode;
    d_inode.nlink = 0;
    d_inode.uid = getuid();
    d_inode.gid = getgid();
    memset(d_inode.xattrs, 0, XATTR_COUNT*sizeof(xattr_t));
    int status = alloc_blocks(&d_inode, block_count);
    if(status != 0)
        return status;
    d_inode.length = size;

    status = add_block(inumber, &d_inode);
    if (status < 0){
        return status;
    }
    return 0;
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
  assert(inode != NULL);
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

void inode_close(inode_t* inode){
    if(inode == NULL)
        return;

    inode->open_count--;
    if(inode->open_count == 0){
        list_remove(&inode->elem);
        free(inode);
    }
}

int grow_inode(inode_t* inode, size_t final_size){
    size_t block_count = bytes_to_nblock( final_size - ilen(inode)) - 1;
    // exit(0);
    return alloc_blocks(&inode->d_inode, block_count);
}

void* cache_buf[BLOCK_SIZE];
inumber_t inum = 0;
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
    int new_block = 0;
    while(size > 0){
        new_block = 0;
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

        // if(offset + write_size >= ilen(inode)){
        if(bytes_to_nblock(offset + write_size) > bytes_to_nblock(ilen(inode))){
            if(written == 0 && bytes_to_nblock(offset + write_size) > 1 + bytes_to_nblock(ilen(inode))){
                status = grow_inode(inode, offset + write_size);
                #ifdef DEBUG_WRITE
                    // printf("adding %lu blocks\n", block_count);
                #endif
                if (status == -1){
                    printf("error while adding new block\n");
                    return -1;
                }
            }
            new_block = 1;
        }


        if (off_in_block == 0 && write_size == BLOCK_SIZE){
            status = 0;
        } else if (new_block == 0){
            status = get_block(block_to_inumber(inode->inumber, block_index), temp_buf);
        } else{
            memset(temp_buf, 0, BLOCK_SIZE);
        }

        if(status != 0){
            printf("error in writing while getting %llu inumber\n", block_to_inumber(inode->inumber, block_index));
            return -1;
        }

        memcpy(temp_buf + off_in_block, buf + written, write_size);
        
        if(new_block == 0)
            status = update_block(block_to_inumber(inode->inumber, block_index), temp_buf);
        else
            status = add_block(block_to_inumber(inode->inumber, block_index), temp_buf);

        if(inum == block_to_inumber(inode->inumber, block_index))
            inum = 0;
            // memcpy(cache_buf, temp_buf, BLOCK_SIZE);

        if(status != 0)
            return status;

        if(offset + write_size > ilen(inode))
            inode->d_inode.length += (write_size - ilen(inode) + offset);
        #ifdef DEBUG_WRITE
            printf("new size:%lu\n", ilen(inode));
            printf("\n");
        #endif
        size -= write_size;
        written += write_size;
        offset += write_size;
    }
    status = update_block(inode->inumber, &inode->d_inode);
    if (status < 0)
        return status;
  return written;
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
            // memset(buf + read, 0, size);

            #ifdef DEBUG_READ
                printf("offset %lu\n", offset);
                printf("inode length: %lu\n", inode->d_inode.length);            
                printf("get out\n");
            #endif

            return read;
        }


        block_index = bytes_to_index(offset);
        off_in_block = offset % BLOCK_SIZE;
        left_in_block = BLOCK_SIZE - off_in_block;

        // read size must be a mininum between
        // what is left to read, what is remaining in 
        // current block and what is remaining in inode
        read_size = left_in_block > size ? size : left_in_block;
        
     

        if(inum == block_to_inumber(inode->inumber, block_index))
            memcpy(temp_buf, cache_buf, BLOCK_SIZE);
        else 
            status = get_block(block_to_inumber(inode->inumber, block_index), temp_buf);
            // printf("block NO: %llu\n", block_to_inumber(inode->inumber, block_index));
        if(status != 0){
            printf("error while reading %llu inumber\n", inode->inumber);
            return -1;
        }

        if(inum != block_to_inumber(inode->inumber, block_index)){
            memcpy(cache_buf, temp_buf, BLOCK_SIZE);
            inum = block_to_inumber(inode->inumber, block_index);
        }

        memcpy(buf + read, temp_buf + off_in_block, read_size);

        size -= read_size;
        read += read_size;
        offset += read_size;    
        //debug
        #ifdef DEBUG_READ
            printf("block_index: %lu\n", block_index);
            printf("off_in_block: %lu\n", off_in_block);
            printf("left_in_block: %lu\n", left_in_block);
            // printf("left_in_inode: %lu\n", left_in_inode);
            printf("left size: %lu\n", size);
            printf("read_size: %lu\n", read_size);
            printf("offset: %lu\n", offset);
            printf("\n");
        #endif
    }
  return read;
}

int inode_delete(inode_t* inode){
    if(inode == NULL)
        return -1;
    if(inode->d_inode.nlink > 1)
        return -1;

    if(inode->open_count > 1)
        return -1;
    // inumber_t max_index = block_to_inumber(inode->inumber, bytes_to_index(ilen(inode)));
    size_t size = bytes_to_nblock(ilen(inode)) + 1;
    int status = 0;
    inumber_t inum = inode->inumber;
    for(inumber_t i = 0; i < size; i++){
        status = remove_block(i + inum);
        if(status != 0){
            printf("error while removing %llu block\n", i);
        }
    }
    inode_close(inode);
    return status;
}

size_t ilen(inode_t* inode){
    return inode->d_inode.length;
}

int inode_chmod(inode_t* inode, int mode){
    if (inode == NULL)
        return -1;
    inode->d_inode.mode = mode;
    if (update_block(inode->inumber, &inode->d_inode) < 0)
        return -1;
    else 
        return 0;
}

int increase_nlink(inode_t* inode){
    if(inode == NULL)
        return -1;
    inode->d_inode.nlink++;
    return update_block(inode->inumber, &inode->d_inode);
}

int decrease_nlink(inode_t* inode){
    if(inode == NULL)
        return -1;
    inode->d_inode.nlink--;
    return update_block(inode->inumber, &inode->d_inode);
}

xattr_t* inode_get_xattr(inode_t* inode, const char* key){
    if(inode == NULL || key == NULL || strlen(key) == 0)
        return NULL;
    xattr_t *xattrs = inode->d_inode.xattrs;
    for(int i = 0; i < XATTR_COUNT; i++){
        if(xattrs[i].size != 0 && strcmp(xattrs[i].key, key) == 0)
            return xattrs + i;
    }
    return NULL;
}

int inode_set_xattr(inode_t* inode, const char* key, const char* value, size_t size){
    if(size > 128 || size == 0)
        return -1;

    if(inode == NULL || key == NULL || strlen(key) == 0 || 
        value == NULL || strlen(value) == 0)
        return -1;
    xattr_t *xattrs = inode->d_inode.xattrs;
    int free = -1;
    int found = -1;
    for(int i = 0; i < XATTR_COUNT; i++){
        if(strcmp(xattrs[i].key, key) == 0  && xattrs[i].size != 0){
            found = i;
            break;
        }
        if(free == -1 && xattrs[i].size == 0){
            free = i;
        }
    }
    int to_use;
    if(found != -1){
        to_use = found;
    } else if (free != -1){
        to_use = free;
    } else{
        return -ENOMEM;
    }
    xattr_t saved_xattr;
    memcpy(&saved_xattr, &xattrs[to_use], sizeof(xattr_t));

    strcpy(xattrs[to_use].key, key);
    memcpy(xattrs[to_use].value, value, size);
    xattrs[to_use].size = (char)size;
    int status = update_block(inode->inumber, &inode->d_inode);
    if(status < 0){
        memcpy(&xattrs[to_use], &saved_xattr, sizeof(xattr_t));
        return status;
    }
    return status;
}

int inode_remove_xattr(inode_t* inode, const char* key){
    if(inode == NULL || key == NULL || strlen(key) == 0)
        return -1;
    xattr_t *xattrs = inode->d_inode.xattrs;
    for(int i = 0; i < XATTR_COUNT; i++){
        if(xattrs[i].size != 0)
        if(strcmp(xattrs[i].key, key) == 0){
            int saved_size = xattrs[i].size;
            xattrs[i].size = 0;
            int status = update_block(inode->inumber, &inode->d_inode);
            if(status < 0){
                xattrs[i].size = saved_size;
                return -1;
            }
            return 0;
        }
    }
    return 0;
}

size_t inode_xattr_list_len(inode_t* inode){
    if(inode == NULL)
        return 0;
    size_t len = 0;
    xattr_t *xattrs = inode->d_inode.xattrs;
    for(int i = 0; i < XATTR_COUNT; i++){
        if(xattrs[i].size != 0)
            len += (strlen(xattrs[i].key) + 1);
    }
    return len;
}

void inode_xattr_list(inode_t* inode, char* list){
    if(inode == NULL)
        return;
    xattr_t *xattrs = inode->d_inode.xattrs;
    char* temp = list;
    for(int i = 0; i < XATTR_COUNT; i++){
        if(xattrs[i].size != 0){
            memcpy(temp, xattrs[i].key, strlen(xattrs[i].key));
            temp += strlen(xattrs[i].key);
            temp[0] = 0;
            temp += 1;
        }
    }
    // list[strlen(list) ] = 0;
}


