#include "filesys.h"
#include <string.h>
#include "free-map.h"
#include "memcached_client.h"
#include <stdio.h>
#include <sys/stat.h>

#include <assert.h>

// TODO: retrieve filesys files from memcached.
void filesys_init(){
    init_connection();
    init_inode();
    init_free_map();

    flush_all();//******************temporary

    dir_create_root();
    cur_dir = dir_open_root();
}

void filesys_finish(){
    dir_close(cur_dir);
    close_connection();
}

// TODO: need to close directories correctly
dir_t* follow_path(char* path, char** recv_file_name){
    if(path == NULL || strlen(path) < 1 )
        return NULL;
    char path_temp[strlen(path) + 1];
    memcpy(path_temp, path, strlen(path) + 1);
    
    char* delim = "/";
    char* token = strtok(path_temp, delim);
    char* temp_token;

    dir_t* c_dir;

    if(path[0] == '/'){
        c_dir = dir_open_root();
        if(token == NULL){
            return c_dir;
        }
    }else{
        c_dir = dir_reopen(cur_dir);
    }

    int status = 0;
    int mode = 0;
    while(token != NULL){
        mode = dir_get_entry_mode(c_dir, token);
        temp_token = strtok(NULL, delim);
        
        // if token is last part
        if(temp_token == NULL){
            *recv_file_name = path + (token - path_temp);
            return c_dir;
        }

        if(!S_ISDIR(mode)){
        // printf("%s**\n", token);
            dir_close(c_dir);
            return NULL;
        }
        c_dir = dir_open(c_dir, token);

        token = temp_token;

    }
    // shouldn't come here
    assert(0);
    return NULL;
}

file_info_t* open_file (char* file_name){
    file_info_t* fi = malloc(sizeof(file_info_t));
}


void close_file (file_info_t* fi){
    inode_close(fi->inode);
    free(fi);
}

int write_file_at (file_info_t* fi, void* buf, size_t off, size_t len){
    return inode_write(fi->inode, buf, off, len);
}
int read_file_at (file_info_t* fi, void* buf, size_t off, size_t len){
    return inode_read(fi->inode, buf, off, len);
}
int write_file (file_info_t* fi, void* buf, size_t len){
    int status = inode_write(fi->inode, buf, fi->pos, len);
    if (status != -1){
        fi->pos += len;
        return status;
    }
    return -1;
}
int read_file (file_info_t* fi, void* buf, size_t len){
    int status = inode_read(fi->inode, buf, fi->pos, len);
    if (status != -1){
        fi->pos += len;
        return status;
    }
    return -1;
}
void seek_file (file_info_t* fi, size_t new_pos){
    fi->pos = new_pos;
}

int getattr_file (file_info_t* fi){
    return fi->inode->d_inode.mode;
}

int getattr_path (char* path){
    char* file_name;
    dir_t* dir = follow_path(path, &file_name);
    int mode = dir_get_entry_mode(dir, file_name);
    dir_close(dir);
    return mode;
}

int get_file_size(char* path){
    char* file_name;
    dir_t* dir = follow_path(path, &file_name);
    if(dir == NULL)
        return -1;
    int size = dir_get_entry_size(dir, file_name);
    dir_close(dir);
    return size;
}





int create_file (char* file_name, size_t size);
int delete_file ();


int filesys_mkdir(char* path){
    char *name;
    dir_t* f_dir = follow_path(path, &name);
    inumber_t inumber = alloc_inumber();
    int status = dir_create(inumber);
    if (status != 0){
        printf("couldn't create dir \n");
        return -1;
    }
    status = dir_add_entry(f_dir, name, inumber, __S_IFDIR);
    dir_t* dir = dir_open(f_dir, name);
    dir_add_entry(dir, ".", inumber, __S_IFDIR);
    dir_add_entry(dir, "..", f_dir->inode->inumber, __S_IFDIR);
    dir_close(dir);
    dir_close(f_dir);
    return status;
}

int filesys_chdir(char* path){
    char* dir_name;
    dir_t* dir = follow_path(path, &dir_name);
    if(dir == NULL)
        return -1;
    dir_t* res_dir = dir_open(dir, dir_name);
    if(res_dir == NULL)
        return -1;
    // TODO: root dir closing needs testing
    dir_close(cur_dir);
    cur_dir = res_dir;
    return 0;
}

dir_t* filesys_opendir(char* path){
    if(strcmp(path, "/") == 0)
        return dir_open_root();
    char* dir_name;
    dir_t* dir = follow_path(path, &dir_name);
    if(dir == NULL)
        return NULL;
    dir_t* res_dir = dir_open(dir, dir_name);
    dir_close(dir);
    return res_dir;
}

int main5(){
    printf("*** real mode %d\n", __S_IFDIR);

    filesys_init();
    filesys_mkdir("/hi");
    readdir_full(cur_dir);

    printf("***mode %d\n", getattr_path("/hi/hello"));
    assert(filesys_mkdir("/hi/hello") == 0);
    
    assert(filesys_chdir("/hi") == 0);
    readdir_full(cur_dir);

    assert(filesys_chdir("/hi/hello") == 0);
    readdir_full(cur_dir);
    dir_reset_seek(cur_dir);

    filesys_mkdir("inside_hello");
    // assert(filesys_chdir("/hi/hello") == 0);

    readdir_full(cur_dir);

    
    // dir_t* dir = dir_open(cur_dir, "hi");
    // readdir_full(dir);
    // printf("_________________\n");

    // dir_t* back = dir_open(dir, ".");
    // readdir_full(back);
    // printf("_________________\n");

    // back = dir_open(dir, "..");
    // readdir_full(back);
    // printf("_________________\n");

    return 0;
}