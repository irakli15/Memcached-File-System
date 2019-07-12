#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include "inode.h"
#define NAME_MAX_LEN 200

struct dir{
    inode_t* inode;
    size_t pos;
};

struct dir_entry{
    inumber_t inumber;
    char name[NAME_MAX_LEN + 1];
    int in_use;
    int mode;
};

typedef struct dir dir_t;
typedef struct dir_entry dir_entry_t;



dir_t* dir_open(dir_t* dir, char* dir_name);
dir_t* dir_reopen(dir_t* dir);
void dir_close(dir_t* dir);

int dir_create(inumber_t inumber);
int dir_remove(dir_t* dir);

int dir_read(dir_t* dir, char* file_name_buf);
int dir_add_entry(dir_t* dir, char* file_name, inumber_t inumber, int mode);
int dir_remove_entry(dir_t* dir, char* file_name);
int dir_entry_exists(dir_t* dir, char* file_name);

void dir_reset_seek(dir_t* dir);


dir_t* dir_open_root();
int dir_create_root();

void readdir_full(dir_t* dir);

int dir_get_entry_mode(dir_t* dir, char* file_name);
int dir_get_entry_size(dir_t* dir, char* file_name);
inumber_t dir_get_entry_inumber(dir_t* dir, char* file_name);
#endif