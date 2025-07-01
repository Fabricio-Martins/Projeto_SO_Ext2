#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include "../structs.h"

#define BLOCK_SIZE 1024

extern FILE *image_file;
extern char image_path[256];
extern ext2_super_block superblock;
extern ext2_group_desc group_desc;

int load_image(const char *path);
void close_image();

const char *get_current_path();
void reset_path();

ext2_inode read_inode(uint32_t inode_num);
void read_block(uint32_t block_num, void *buffer);

uint32_t get_current_inode();

#endif