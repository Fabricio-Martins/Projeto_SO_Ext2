#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../structs.h"

#define BLOCK_SIZE 1024

FILE *image_file = NULL;
char image_path[256];
ext2_super_block superblock;
ext2_group_desc group_desc;

int load_image(const char *path)
{
    strncpy(image_path, path, sizeof(image_path));
    image_file = fopen(path, "rb+");
    if (!image_file)
    {
        perror("Error: Falha ao abrir o arquivo da imagem.");
        return -1;
    }

    fseek(image_file, 1024, SEEK_SET);
    fread(&superblock, sizeof(ext2_super_block), 1, image_file);

    if (superblock.magic != 0xEF53)
    {
        fprintf(stderr, "Error: Assinatura não é EXT2 válida\n");
        return -1;
    }

    fseek(image_file, 2048, SEEK_SET);
    fread(&group_desc, sizeof(ext2_group_desc), 1, image_file);

    return 0;
}

void close_image()
{
    if (image_file)
        fclose(image_file);
}

static char current_path[1024] = "/";

const char *get_current_path()
{
    return current_path;
}

void reset_path()
{
    strcpy(current_path, "/");
}

ext2_inode read_inode(uint32_t inode_num)
{
    ext2_inode inode;
    uint32_t index = inode_num - 1;
    uint32_t inodes_per_block = BLOCK_SIZE / superblock.inode_size;
    uint32_t index_in_group = index % superblock.inodes_per_group;
    uint32_t block = group_desc.inode_table + (index_in_group / inodes_per_block);
    uint32_t offset = (index_in_group % inodes_per_block) * superblock.inode_size;

    fseek(image_file, block * BLOCK_SIZE + offset, SEEK_SET);
    fread(&inode, sizeof(ext2_inode), 1, image_file);
    return inode;
}

void read_block(uint32_t block_num, void *buffer)
{
    fseek(image_file, block_num * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, image_file);
}

static uint32_t current_inode_num = 2;

uint32_t get_current_inode()
{
    return current_inode_num;
}