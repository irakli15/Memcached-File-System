/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * ## Source code ##
 * \include hello.c
 */


#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include "memcached_client.h"

#include "inode.h"
#include "free-map.h"
#include "memcached_client.h"
#include "directory.h"

#include "filesys.h"

#include <unistd.h>
/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static void *fs_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}

static int fs_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;
	// printf("getattr \n");
		printf("getattr path %s\n", path);
		if(fi != NULL)
	printf("fi: %d\n", fi->fh);

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0777; //0755;
		stbuf->st_nlink = 2;
	} else {
		// stbuf->st_mode = S_IFREG | 0444;
		// stbuf->st_nlink = 1;
		// stbuf->st_size = strlen(options.contents);


		int mode = getattr_path(path);
		if(mode == -1)
			return -ENOENT;
		// dir_t* dir = dir_open(cur_dir, path + 1);
		// printf("%d mine\n", dir_get_entry_mode(cur_dir, path+1) | 0755);
		// printf("%d not mine\n", S_IFDIR | 0755);

		stbuf->st_mode = mode | 0777;//0755;
		stbuf->st_nlink = 1;
		stbuf->st_size = get_file_size(path);
	} 
	// else
	// 	res = -ENOENT;

	return res;
}

int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	printf("create: %s\n", path);
	file_info_t* f_info = create_file(path, mode);
	fi->fh = (uint64_t)f_info;
	if(f_info == NULL)
		return -1;
	return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
	printf("open file\n");

	if (strcmp(path+1, "test") != 0)
		return -ENOENT;

	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	printf("read\n");
	// printf("hi\n");
	size_t len;
	(void) fi;
	if(strcmp(path+1, "test") != 0)
		return -ENOENT;
	char str_buf[10];
	memset(str_buf, 0, 10);
    size = 0;

	return size;
}

static int fs_mkdir(const char *path, mode_t mode)
{
	int res;
	printf("mkdir\n");
	// printf("%s\n", path);
	res = filesys_mkdir(path);
	if (res == -1)
		return -errno;
	
	// readdir_full(filesys_opendir("/hi"));

	return 0;
}

static int fs_rmdir(const char *path){
	printf("rmdir path: %s\n", path);
	return filesys_rmdir(path);
}

static int fs_opendir(const char *path, struct fuse_file_info *fi){
	printf("opendir path: %s\n", path);
	dir_t* dir = filesys_opendir(path);
		if(fi != NULL)
	fi->fh = (uint64_t)dir;
	printf("fi: %d\n", fi->fh);
	if (dir == NULL)
		return -1;
	return 0;
}

static int fs_releasedir(const char *path, struct fuse_file_info *fi){
	printf("releasedir: %s\n", path);
	if(fi != NULL){
		printf("fi: %d\n", fi->fh);
		dir_close((dir_t*)fi->fh);
		return 0;
	}
	return -1;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	printf("readdir path: %s\n", path);
		if(fi != NULL)	
	printf("fi: %d\n", fi->fh);

	// filler(buf, ".", NULL, 0, 0);
	// filler(buf, "..", NULL, 0, 0);
	// filler(buf, options.filename, NULL, 0, 0);
    dir_t* dir = filesys_opendir(path);
	if(dir == NULL)
		return -1;
    char file_name[NAME_MAX_LEN];
    while(dir_read(dir, file_name) == 0){
		printf("%s\n", file_name);
        filler(buf, file_name, NULL, 0, 0);
    }
	dir_close(dir);

	return 0;
}



static struct fuse_operations fs_oper = {
	.init       = fs_init,
	.getattr	= fs_getattr,
	.readdir	= fs_readdir,
	.opendir	= fs_opendir,
	.releasedir = fs_releasedir,
	.create		= fs_create,
	.open		= fs_open,
	.read		= fs_read,
	.mkdir 		= fs_mkdir,
	.rmdir		= fs_rmdir
};



static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --name=<s>          Name of the \"hello\" file\n"
	       "                        (default: \"hello\")\n"
	       "    --contents=<s>      Contents \"hello\" file\n"
	       "                        (default \"Hello, World!\\n\")\n"
	       "\n");
}

int main(int argc, char *argv[])
{
    filesys_init();
    filesys_mkdir("/hi");
    filesys_mkdir("/hi/ho");
	filesys_mkdir("/hey");

	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	// options.filename = strdup("hello");
	// options.contents = strdup("Hello World!\n");

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

	ret = fuse_main(args.argc, args.argv, &fs_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}