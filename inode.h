#ifndef _INODE_H
#define _INODE_H

#define SECTOR_SIZE 1024
#define DBL_IND_COUNT 16
#define DIRECT_COUNT 235

#define FREE_MAP_SECTOR 0
#define ROOT_DIR_SECTOR 1

typedef unsigned long long inumber_t;
typedef unsigned int sector_t;
typedef unsigned int uint_size_t;

struct disk_inode{
  inumber_t inumber; //8
  uint_size_t length; //4
  sector_t direct[DIRECT_COUNT]; //
  sector_t ind; //4
  sector_t dbl_ind[DBL_IND_COUNT]; //16*4
  int mode;
};

void init_inode();
int inode_create(sector_t sector, uint_size_t size, int mode);


#endif