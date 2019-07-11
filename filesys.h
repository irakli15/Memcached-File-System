#include <stdlib.h>
#include "inode.h"
#include "directory.h"
#include "list.h"

struct file_info {
    inode_t* inode;
    size_t pos;
    // struct list_elem elem;
};

typedef struct file_info file_info_t;
dir_t* cur_dir;

void filesys_init();

file_info_t* open_file (char* file_name);
void close_file (file_info_t* fi);
int write_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int read_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int write_file (file_info_t* fi, void* buf, size_t len);
int read_file (file_info_t* fi, void* buf, size_t len);
void seek_file (file_info_t* fi, size_t new_pos);
int getattr_file (file_info_t* fi);


int create_file (char* file_name, size_t size);
int delete_file ();

int filesys_chmod(char* path);
int filesys_chdir(char* path);

int filesys_mkdir(char* path);
// int rename_file ();