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

file_info_t* open_file (const char* path);
void close_file (file_info_t* fi);
int write_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int read_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int write_file (file_info_t* fi, void* buf, size_t len);
int read_file (file_info_t* fi, void* buf, size_t len);
void seek_file (file_info_t* fi, size_t new_pos);
void reset_file_seek (file_info_t* fi);
file_info_t* create_file (const char* path, uint64_t mode);
int delete_file ();

int getattr_inode (inode_t* inode);
int getattr_path (const char* path);
int get_file_size(const char* path);



int filesys_chmod(const char* path);
int filesys_chdir(const char* path);

int filesys_mkdir(const char* path);
dir_t* filesys_opendir(const char* path);
int filesys_rmdir(const char* path);

char* get_last_part(const char* path);

// int rename_file ();