#include <stdlib.h>
#include "inode.h"
#include "directory.h"
#include "list.h"

// permissions
#define RD      "r00"
#define WR      "0w0"
#define EX      "00x"
#define RDEX    "r0x"
#define WREX    "0wx"
#define RDWR    "rw0"
#define RDWREX  "rwx"


struct file_info {
    inode_t* inode;
    // struct list_elem elem;
};

typedef struct file_info file_info_t;
dir_t* cur_dir;

void filesys_init();
void filesys_finish();

file_info_t* open_file (const char* path);
void close_file (file_info_t* fi);
int write_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int read_file_at (file_info_t* fi, void* buf, size_t off, size_t len);
int create_file (const char* path, uint64_t mode, file_info_t **f_info);
int delete_file (const char* path);

int getattr_inode (inode_t* inode);
int getattr_path (const char* path);
int get_file_size(const char* path);



int filesys_chdir(const char* path);

int filesys_mkdir(const char* path, int mode);
dir_t* filesys_opendir(const char* path);
int filesys_rmdir(const char* path);

char* get_last_part(const char* path);

dir_t* follow_path(const char* path, char** recv_file_name);

int check_permission(inode_t* inode, char* perms);

// int rename_file ();