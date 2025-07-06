#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../../utils/utils.h"
#include "../../structs.h"
#include "ls.h"

void ls_command(void) {
    ext2_inode inode; // diretório atual

    if (get_current_inode(&inode) != 0) {
        fprintf(stderr, "Erro ao obter o inode atual.\n");
        return;
    }

    if (!S_ISDIR(inode.mode)) {
        fprintf(stderr, "Erro: inode atual não é um diretório.\n");
        return;
    }

    void *block = malloc(BLOCK_SIZE);
    if (!block) {
        fprintf(stderr, "Erro de memória\n");
        return;
    }


    read_block(inode.block[0], block);

    ext2_dir_entry *entry = (ext2_dir_entry *)block;
    unsigned int size = 0;

    while (size < inode.size && entry->inode) {
        char name[NAME_LENGHT + 1];
        memcpy(name, entry->name, entry->name_len);
        name[entry->name_len] = '\0';

        printf("%s\n", name);

        size += entry->rec_len;
        entry = (void*)entry + entry->rec_len;
    }

    free(block);
}
