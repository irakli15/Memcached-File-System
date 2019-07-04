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

#define NAME_MAX_LEN 20
int dir_lookup_entry(dir_t* dir, char* file_name, size_t* offset, dir_entry_t* dir_entry);
int find_free_entry(dir_t* dir, size_t* offset);
int check_file_name(char* file_name);


struct dir{
    inode_t* inode;
    size_t pos;
};

struct dir_entry{
    inumber_t inumber;
    char name[NAME_MAX_LEN + 1];
    int in_use;
};

dir_t* dir_open(inode_t* inode){
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


int dir_create(inumber_t inumber, size_t entry_count){
    return inode_create(inumber, entry_count*sizeof(dir_entry_t), __S_IFDIR);
}
int dir_remove(dir_t* dir){

    // count entries in dir
    // if empty, then delete

    // inode_delete(dir->inode);
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

int dir_add_entry(dir_t* dir, char* file_name, inumber_t inumber){
    if(check_file_name(file_name) != 0)
        return -1;
    int status = dir_lookup(dir, file_name, NULL);
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
    
    return status;
}

int dir_remove_entry(dir_t* dir, char* file_name){
    if(check_file_name(file_name) != 0)
        return -1;
    dir_entry_t dir_entry;
    size_t offset;
    int status = dir_lookup_entry(dir, file_name, &offset, &dir_entry);
    if (status != 0){
        return -1;
    }
    dir_entry.in_use = 0;
    status = inode_write(dir->inode, &dir_entry, offset, sizeof(dir_entry));
    if(status != 0){
        printf("failed to remove  %s\n", file_name);
        return -1;
    }
    return 0;
}

int dir_lookup_entry(dir_t* dir, char* file_name, size_t* offset, dir_entry_t* dir_entry){
    if(check_file_name(file_name) != 0)
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

// looks for file_name, if found inumber will be filled
// and returned 0, else -1
int dir_lookup(dir_t* dir, char* file_name, inumber_t* inumber){
    dir_entry_t dir_entry;
    int status = dir_lookup_entry(dir, file_name, NULL, &dir_entry);

    if (status == 0){
        if(inumber != NULL){
            *inumber = dir_entry.inumber;
        }
        return 0;
    }
    return -1;
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

void readdir_full(dir_t* dir){
    char buff[NAME_MAX_LEN];
    int status;
        while(1){
        status = dir_read(dir, buff);
        if(status == -1){
            printf("error while reading\n");
            break;
        }else if(status == 1){
            printf("reached end\n");
            break;
        }
        printf("file_name: %s\n", buff);
    }
}

void basic_dir_create_read_test(){
    flush_all();    
    inumber_t inumber = alloc_inumber();
    int status = dir_create(inumber,0);
    // printf("%d\n", 3<<4);
    printf("create status %d\n", status);
    dir_t* dir = dir_open(inode_open(inumber));
    status = dir_add_entry(dir, "file1", alloc_inumber());
    printf("add file1 status %d\n", status);
    status = dir_add_entry(dir, "file2", alloc_inumber());
    printf("add file2 status %d\n", status);
    status = dir_add_entry(dir, "file3", alloc_inumber());
    printf("add file3 status %d\n", status);

    status = dir_add_entry(dir, "file3", alloc_inumber());
    printf("add file3 again status %d\n", status);
    readdir_full(dir);


    status = dir_remove_entry(dir, "file2");
    printf("remove file2 status %d\n", status);
    dir_reset_seek(dir);
    readdir_full(dir);
    status = dir_add_entry(dir, "file4", alloc_inumber());
    printf("add file4 status %d\n", status);

    dir_remove_entry(dir, "file1");
    dir_reset_seek(dir);
    readdir_full(dir);

    dir_remove_entry(dir, "file3");
    dir_reset_seek(dir);
    readdir_full(dir);

    dir_remove_entry(dir, "file4");
    dir_reset_seek(dir);
    readdir_full(dir);

    status = dir_add_entry(dir, "file4", alloc_inumber());
    printf("add file4 again status %d\n", status);
}

void add_entries(dir_t* dir, int count){
    inumber_t inumber;
    char file_name[10];
    int status;
    while(count > 0){
        inumber = alloc_inumber();
        sprintf(file_name, "file%llu", inumber>>4 );
        status = dir_add_entry(dir, file_name, inumber);
        assert(status == 0);
        count--;
    }
}

void dir_stress_test(){
    flush_all();
    inumber_t dir_inumber = alloc_inumber();
    int status = dir_create(dir_inumber, 0);
    dir_t* dir = dir_open(inode_open(dir_inumber));
    assert(status == 0);
    add_entries(dir, 5000/sizeof(dir_entry_t));
    readdir_full(dir);
}


int main(){
    init_connection();
    init_inode();
    init_free_map();
    flush_all();
    // basic_dir_create_read_test();
    // dir_stress_test();
    inumber_t dir_inumber = alloc_inumber();
    int status = dir_create(dir_inumber, 2);
    dir_t* dir = dir_open(inode_open(dir_inumber));
    dir_add_entry(dir, "file", alloc_inumber());
    dir_add_entry(dir, "file1", alloc_inumber());
    dir_add_entry(dir, "file2", alloc_inumber());

    printf("%lu\n", ilen(dir->inode));
    return 0;
}