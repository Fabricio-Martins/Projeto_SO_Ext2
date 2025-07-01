#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cat.h"
#include "../../utils/utils.h"

void cat_command(const char *file)
{
    ext2_inode dir_inode = read_inode(get_current_inode());
    uint8_t block[BLOCK_SIZE];
    read_block(dir_inode.block[0], block);

    int offset = 0;
    while (offset < BLOCK_SIZE)
    {
        ext2_dir_entry *entry = (ext2_dir_entry *)(block + offset);
        if (entry->inode == 0)
            break;

        char name[256];
        memcpy(name, entry->name, entry->name_len);
        name[entry->name_len] = '\0';

        if (strcmp(name, file) == 0)
        {
            ext2_inode file_inode = read_inode(entry->inode);
            if ((file_inode.mode & 0xF000) != 0x8000)
            {
                printf("cat: '%s' não é um arquivo\n", file);
                return;
            }

            int bytes_remaining = file_inode.size;
            for (int i = 0; i < 12 && bytes_remaining > 0; i++)
            {
                if (file_inode.block[i] == 0)
                    break;
                read_block(file_inode.block[i], block);
                int bytes_to_read = bytes_remaining > BLOCK_SIZE ? BLOCK_SIZE : bytes_remaining;
                fwrite(block, 1, bytes_to_read, stdout);
                bytes_remaining -= bytes_to_read;
            }
            printf("\n");
            return;
        }
        offset += entry->rec_len;
    }

    printf("cat: '%s' arquivo inexistente\n", file);
}
