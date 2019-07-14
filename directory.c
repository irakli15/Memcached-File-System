#include "directory.h"
#include "inode.h"
#include <assert.h>
#include <string.h>
// #include <sys/types.h>
#include <sys/stat.h>

//testing
#include "memcached_client.h"
#include <stdio.h>
#include "free-map.h"
#include <errno.h>

int dir_lookup_entry(dir_t* dir, char* file_name, size_t* offset, dir_entry_t* dir_entry);
dir_entry_t* dir_lookup(dir_t* dir, char* file_name);
int find_free_entry(dir_t* dir, size_t* offset);
int check_file_name(char* file_name);
dir_t* get_dir(inode_t* inode);


dir_t* dir_open(dir_t* dir, char* dir_name){
    if(strcmp(dir_name, "/")==0)
        return dir_open_root();
    dir_entry_t* dir_entry = dir_lookup(dir, dir_name);
    if(dir_entry == NULL)
        return NULL;
    inode_t* inode = inode_open(dir_entry->inumber);
    if(inode == NULL)
        return NULL;
    if(!S_ISDIR(inode->d_inode.mode)){
        inode_close(inode);
        return NULL;
    }
    dir_t* res_dir = get_dir(inode);
    free(dir_entry);
    return res_dir;
}

dir_t* get_dir(inode_t* inode){
    if(inode == NULL)
        return NULL;
    dir_t* dir = malloc(sizeof(dir_t));
    if(dir == NULL)
        return NULL;
    dir->inode = inode;
    dir->pos = 0;
    return dir;
}

void dir_close(dir_t* dir){
    inode_close(dir->inode);
    free(dir);
}


int dir_create(inumber_t inumber){
    return inode_create(inumber, 0, __S_IFDIR);
}

int dir_create_root(){
    int status = inode_create(ROOT_DIR_INUMBER, 2*sizeof(dir_entry_t), __S_IFDIR);
    if(status != 0){
        printf("couldn't create root dir\n");
        return -1;
    }
    dir_t* dir = dir_open_root(ROOT_DIR_INUMBER);
    dir_add_entry(dir, ".", ROOT_DIR_INUMBER, __S_IFDIR);

    dir_close(dir);
}

dir_t* dir_open_root(){
    inode_t* inode = inode_open(ROOT_DIR_INUMBER);
    return get_dir(inode);
}



int dir_remove(dir_t* dir){
    if(dir == NULL)
        return -1;
    if(dir->inode->inumber == ROOT_DIR_INUMBER)
        return -1;
        
    if(dir->inode->open_count > 1)
        return -EBUSY;
    
    char file_name[NAME_MAX_LEN];
    while(dir_read(dir, file_name) == 0){
        if(strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0){
            continue;
        }
        return -ENOTEMPTY;
    }
    int status = inode_delete(dir->inode);
    free(dir);
    return status;
}
// 1 means reached the end for now
int dir_read(dir_t* dir, char* file_name_buf){
    if(file_name_buf == NULL)
        return -1;
    if(dir->pos + sizeof(dir_entry_t) > ilen(dir->inode)){
        *file_name_buf = 0;
        return 1;
    }

    size_t pos = dir->pos;
    dir_entry_t entry;
    int status = 0;
    while(pos + sizeof(dir_entry_t) <= ilen(dir->inode)){
        status = inode_read(dir->inode, &entry, pos,sizeof(dir_entry_t));
        pos += sizeof(dir_entry_t);
        dir->pos = pos;
        if(status != 0){
            printf("error while reading from dir\n");
            return -1;
        }
        if(entry.in_use == 1){
            strncpy(file_name_buf, entry.name, strlen(entry.name));
            file_name_buf[strlen(entry.name)] = 0;
            return 0;
        }
    }
    *file_name_buf = 0;
    return 1;
}

int dir_add_entry(dir_t* dir, char* file_name, inumber_t inumber, int mode){
    if(check_file_name(file_name) != 0)
        return -1;
    int status = dir_entry_exists(dir, file_name);
    if(status == 0){
        printf("error, file name: %s already exists\n", file_name);
        return -1;
    }

    size_t offset;
    status = find_free_entry(dir, &offset);
    if(status != 0){
        offset = ilen(dir->inode);
    }
    dir_entry_t dir_entry;
    dir_entry.in_use = 1;
    dir_entry.inumber = inumber;
    strncpy(dir_entry.name, file_name, strlen(file_name));
    dir_entry.name[strlen(file_name)] = 0;

    status = inode_write(dir->inode, &dir_entry, offset, sizeof(dir_entry_t));

    if(status != 0)
        printf("couldn't add entry to dir\n");
    
    if(status == 0 && !S_ISDIR(mode)){
        inode_t* inode = inode_open(inumber);
        if(inode == NULL)
            return -1;
        inode->d_inode.nlink++;
        status = update_block(inode->inumber, &inode->d_inode);
        inode_close(inode);
    }
    
    return status;
}

int dir_remove_entry(dir_t* dir, char* file_name, int mode){
    if(check_file_name(file_name) != 0)
        return -1;
    if(strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0)
        return -1;
        
    dir_entry_t dir_entry;
    size_t offset;
    int status = dir_lookup_entry(dir, file_name, &offset, &dir_entry);
    if (status != 0){
        return -1;
    }
    inumber_t inumber = dir_entry.inumber;
    dir_entry.in_use = 0;
    status = inode_write(dir->inode, &dir_entry, offset, sizeof(dir_entry));
    if(status != 0){
        printf("failed to remove  %s\n", file_name);
        return -1;
    }

    // mustn't open inode if it is dir, as it will already have been removed.
    if(status == 0 && !S_ISDIR(mode)){
        inode_t* inode = inode_open(inumber);
        inode->d_inode.nlink--;
        status = update_block(inode->inumber, &inode->d_inode);
        inode_close(inode);
    }

    return 0;
}

int dir_lookup_entry(dir_t* dir, char* file_name, size_t* offset, dir_entry_t* dir_entry){
    if(check_file_name(file_name) != 0)
        return -1;
    if(dir == NULL)
        return -1;

    size_t pos = 0;
    dir_entry_t entry;
    int status = 0;
    while(pos + sizeof(dir_entry_t) <= ilen(dir->inode)){
        status = inode_read(dir->inode, &entry, pos, sizeof(dir_entry_t));
        if(status != 0){
            printf("error reading dir w/inumber:%llu\n", dir->inode->inumber);
            return -1;
        }
        
        if(entry.in_use == 1 && strcmp(entry.name, file_name) == 0){
            if(offset != NULL)
                *offset = pos;
            if(dir_entry != NULL)
                memcpy(dir_entry, &entry, sizeof(dir_entry_t));
            return 0;
        }
        pos += sizeof(dir_entry_t);
    }
    return -1;
}

dir_entry_t* dir_lookup(dir_t* dir, char* file_name){
    dir_entry_t* dir_entry = malloc(sizeof(dir_entry_t));
    if(dir_entry == NULL)
        return NULL;

    int status = dir_lookup_entry(dir, file_name, NULL, dir_entry);

    if (status == 0){
        return dir_entry;
    }
    free(dir_entry);
    return NULL;
}

int dir_entry_exists(dir_t* dir, char* file_name){
    return dir_lookup_entry(dir, file_name, NULL, NULL); 
}

int find_free_entry(dir_t* dir, size_t* offset){
    size_t pos = 0;
    dir_entry_t entry;
    int status = 0;
    while(pos + sizeof(dir_entry_t) <= ilen(dir->inode)){
        status = inode_read(dir->inode, &entry, pos, sizeof(dir_entry_t));
        if(status != 0){
            printf("error reading dir w/inumber:%llu\n", dir->inode->inumber);
            return -1;
        }
        if(entry.in_use == 0){
            if(offset != NULL)
                *offset = pos;
            return 0;
        }
        pos += sizeof(dir_entry_t);
    }
    return -1;
}

int check_file_name(char* file_name){
    if(file_name == NULL || 
        strlen(file_name) == 0 || 
        strlen(file_name) > NAME_MAX_LEN){
            printf("invalid file name\n");
            return -1;
        }
    return 0;
}

void dir_reset_seek(dir_t* dir){
    if(dir != NULL)
        dir->pos = 0;
}

int dir_get_entry_mode(dir_t* dir, char* file_name){
    dir_entry_t dir_entry;
    if(dir == NULL || file_name == NULL)
        return -1;
        
    // int status = dir_lookup_entry(dir, file_name, NULL, &dir_entry);
    if(dir_entry_exists(dir, file_name) != 0)
        return -1;

    inode_t* inode = inode_open(dir_get_entry_inumber(dir, file_name));
    if(inode == NULL)
        return -1;
    int mode = inode->d_inode.mode;
    inode_close(inode);
    return mode;
        // printf("%s**\n", file_name);

        // printf("%d\n", status);

    // if (status == 0){
    //    return dir_entry.mode;
    // }
    // return -1;
}

dir_t* dir_reopen(dir_t* dir){
    dir_t* new_dir = malloc(sizeof(dir_t));
    new_dir->inode = inode_open(dir->inode->inumber);
    new_dir->pos = 0;
    return new_dir;
}

int dir_get_entry_size(dir_t* dir, char* file_name){
    if(dir == NULL || check_file_name(file_name) != 0)
        return -1;
    dir_entry_t* entry = dir_lookup(dir, file_name);
    if(entry == NULL)
        return -1;
    inode_t* inode = inode_open(entry->inumber);
    if(inode == NULL)
        return -1;
    int size = ilen(inode);
    inode_close(inode);
    free(entry);
    return size;
}

inumber_t dir_get_entry_inumber(dir_t* dir, char* file_name){
    if(dir == NULL || check_file_name(file_name) != 0)
        return 0;
    dir_entry_t* entry = dir_lookup(dir, file_name);
    if(entry == NULL)
        return 0;
    inumber_t inumber = entry->inumber;
    free(entry);
    return inumber;
}


void readdir_full(dir_t* dir){
    char buff[NAME_MAX_LEN + 1];
    int status;
    
    printf("------------------\n");

    while(1){
        status = dir_read(dir, buff);
        if(status == -1){
            printf("error while reading\n");
            break;
        }else if(status == 1){
            break;
        }
        printf("%s\n", buff);
    }
    printf("=================\n");
}