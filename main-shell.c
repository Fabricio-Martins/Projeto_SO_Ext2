#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"
#include "cmd/info/info.h"
#include "cmd/cat/cat.h"

#define CMD_BUFFER_SIZE 128

int main(int argc, char *argv[])
{
    if (load_image(argv[1]) != 0)
    {
        fprintf(stderr, "Error: Falha ao carregar ou arquivo da imagem não encontrado.\n");
        return 1;
    }

    (void)argc;

    reset_path();

    char cmd[CMD_BUFFER_SIZE];
    printf("Ext2:~%s$ ", get_current_path());

    while (fgets(cmd, sizeof(cmd), stdin))
    {
        cmd[strcspn(cmd, "\n")] = 0;

        if (strcmp(cmd, "exit") == 0)
        {
            break;
        }
        else if (strcmp(cmd, "info") == 0)
        {
            info_command();
        }
        else if (strncmp(cmd, "cat ", 4) == 0)
        {
            char *arg = cmd + 4;
            cat_command(arg);
        }
        else
        {
            printf("Comando \'%s\' não encontrado\n", cmd);
        }

        printf("Ext2:~%s$ ", get_current_path());
    }

    close_image();
    return 0;
}
