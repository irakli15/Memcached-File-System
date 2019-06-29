#include "free-map.h"
sector_t succesive_sector;

// this implementation will suffice for now

void init_free_map(){
    // 0 for free-map file
    // 1 for root dir
    succesive_sector = 2;
}

sector_t alloc_sector(){
    return succesive_sector++;
}

void free_sector(sector_t sector){
    
}
