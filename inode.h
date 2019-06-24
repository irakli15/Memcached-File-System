#define SECTOR_SIZE 1024
#define DBL_IND_COUNT 16
#define DIRECT_COUNT 236
typedef unsigned long long inumber_t;
typedef unsigned int sector_t;
typedef unsigned int uint_size_t;

struct disk_inode{
  inumber_t inumber; //8
  uint_size_t length; //4
  sector_t direct[DIRECT_COUNT]; //
  sector_t ind; //4
  sector_t dbl_ind[DBL_IND_COUNT]; //16*4
};