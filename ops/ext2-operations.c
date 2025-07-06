#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "ext2-operations.h"

// Comando info
int cmd_info(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    (void)argv;
    (void)cwd;

    if (argc != 1)
    {
        errno = EINVAL;
        return EXIT_FAILURE;
    }

    char volume_name[17] = {0};
    memcpy(volume_name, fs->sb.s_volume_name, 16);

    off_t image_size = lseek(fs->fd, 0, SEEK_END);
    if (image_size < 0)
    {
        perror("Erro ao obter tamanho da imagem");
        return EXIT_FAILURE;
    }

    uint32_t free_blocks = fs->sb.s_free_blocks_count;
    uint32_t free_inodes = fs->sb.s_free_inodes_count;
    uint32_t block_size = EXT2_BLOCK_SIZE;
    uint32_t inode_size = fs->sb.s_inode_size;
    uint32_t group_count = fs->groups_count;
    uint32_t blocks_per_group = fs->sb.s_blocks_per_group;
    uint32_t inodes_per_group = fs->sb.s_inodes_per_group;
    uint32_t inodetable_blocks = (inodes_per_group * inode_size + block_size - 1) / block_size;

    printf("Volume name.....: %s\n", volume_name[0] ? volume_name : "sem nome");
    printf("Image size......: %lld bytes\n", (long long)image_size);
    printf("Free space......: %u KiB\n", ((free_blocks - fs->sb.s_r_blocks_count) * block_size) / 1024);
    printf("Free inodes.....: %u\n", free_inodes);
    printf("Free blocks.....: %u\n", free_blocks);
    printf("Block size......: %u bytes\n", block_size);
    printf("Inode size......: %u bytes\n", inode_size);
    printf("Group count.....: %u\n", group_count);
    printf("Groups size.....: %u blocks\n", blocks_per_group);
    printf("Groups inodes...: %u inodes\n", inodes_per_group);
    printf("Inodetable size.: %u blocks\n", inodetable_blocks);
    printf("\n");

    return EXIT_SUCCESS;
}

static void build_perm_string(struct ext2_inode *inode, char out[11])
{
    uint16_t mode = inode->i_mode;

    // Tipo de arquivo
    if ((mode & 0xF000) == EXT2_S_IFDIR)
        out[0] = 'd';
    else if ((mode & 0xF000) == EXT2_S_IFREG)
        out[0] = '-';
    else if ((mode & 0xF000) == EXT2_S_IFLNK)
        out[0] = 'l';
    else if ((mode & 0xF000) == EXT2_S_IFCHR)
        out[0] = 'c';
    else if ((mode & 0xF000) == EXT2_S_IFBLK)
        out[0] = 'b';
    else if ((mode & 0xF000) == EXT2_S_IFIFO)
        out[0] = 'p';
    else if ((mode & 0xF000) == EXT2_S_IFSOCK)
        out[0] = 's';
    else
        out[0] = '?';

    // Permissões: usuário, grupo, outros
    out[1] = (mode & EXT2_S_IRUSR) ? 'r' : '-';
    out[2] = (mode & EXT2_S_IWUSR) ? 'w' : '-';
    out[3] = (mode & EXT2_S_IXUSR) ? 'x' : '-';
    out[4] = (mode & EXT2_S_IRGRP) ? 'r' : '-';
    out[5] = (mode & EXT2_S_IWGRP) ? 'w' : '-';
    out[6] = (mode & EXT2_S_IXGRP) ? 'x' : '-';
    out[7] = (mode & EXT2_S_IROTH) ? 'r' : '-';
    out[8] = (mode & EXT2_S_IWOTH) ? 'w' : '-';
    out[9] = (mode & EXT2_S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

static void human_size(uint32_t bytes, char *out, size_t outsz)
{
    if (bytes < 1024)
        snprintf(out, outsz, "%u B", bytes);
    else if (bytes < 1024 * 1024)
    {
        double k = bytes / 1024.0;
        snprintf(out, outsz, "%.1f KiB", k);
    }
    else
    {
        double m = bytes / 1048576.0;
        snprintf(out, outsz, "%.1f MiB", m);
    }
}

int cmd_attr(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    char *full_path = fs_join_path(fs, *cwd, argv[1]); // Combina o diretório atual com o caminho fornecido
    if (!full_path)
        return -1;

    uint32_t inode_num;
    if (fs_path_resolve(fs, full_path, &inode_num) < 0) // Resolve o caminho para obter o inode correspondente
    {
        fprintf(stderr, "Erro: caminho '%s' não encontrado.\n", argv[1]);
        free(full_path);
        return -1;
    }

    struct ext2_inode inode;
    if (fs_read_inode(fs, inode_num, &inode) < 0) // Lê o inode do arquivo ou diretório
    {
        fprintf(stderr, "Erro ao ler inode.\n");
        free(full_path);
        return -1;
    }

    char perm_str[11];
    build_perm_string(&inode, perm_str); // Constrói a string de permissões a partir do inode

    char size_str[32];
    human_size(inode.i_size, size_str, sizeof(size_str)); // Converte o tamanho do arquivo em uma string legível

    char mod_date[20];
    time_t mod_time = inode.i_mtime;
    struct tm tm_info;
    localtime_r(&mod_time, &tm_info);
    strftime(mod_date, sizeof(mod_date), "%d/%m/%Y %H:%M", &tm_info); // Formata a data de modificação

    printf("%-11s %-6s %-6s %-12s %-17s\n", "permissões", "uid", "gid", "tamanho", "modificado em");
    printf("%-10s %-6u %-6u %-12s %-17s\n",
           perm_str,
           inode.i_uid,
           inode.i_gid,
           size_str,
           mod_date);

    free(full_path);
    return 0;
}

int dump_blk(ext2_fs_t *fs, uint32_t blk, uint32_t nbytes)
{
    static uint8_t zbuf[EXT2_BLOCK_SIZE] = {0}; // Buffer de zeros para blocos vazios
    uint8_t buf[EXT2_BLOCK_SIZE];               // Buffer para leitura do bloco

    const void *src = zbuf;
    if (blk)
    {
        if (fs_read_block(fs, blk, buf) < 0) // Lê o bloco do sistema de arquivos
            return -1;
        src = buf;
    }
    size_t written = fwrite(src, 1, nbytes, stdout); // Escreve o conteúdo
    if (written != nbytes)
    {
        return -1;
    }
    return 0;
}

int dump_file(ext2_fs_t *fs, const struct ext2_inode *in)
{
    uint32_t bytes_left = in->i_size;

    // Blocos diretos
    for (int i = 0; i < 12 && bytes_left > 0; ++i)
    {
        uint32_t to_read = (bytes_left < EXT2_BLOCK_SIZE) ? bytes_left : EXT2_BLOCK_SIZE; // Tamanho a ser lido
        if (dump_blk(fs, in->i_block[i], to_read) < 0)                                    // Lê o bloco do sistema de arquivos
            return -1;
        bytes_left -= to_read;
    }

    // Bloco indireto simples
    if (bytes_left > 0 && in->i_block[12])
    {
        uint8_t buf[EXT2_BLOCK_SIZE];                    // Buffer para leitura do bloco indireto
        if (fs_read_block(fs, in->i_block[12], buf) < 0) // Lê o bloco indireto simples
            return -1;
        uint32_t *indirect = (uint32_t *)buf;
        for (uint32_t i = 0; i < PTRS_PER_BLOCK && bytes_left > 0; ++i) // Lê os blocos indiretos
        {
            uint32_t to_read = (bytes_left > EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : bytes_left; // Tamanho a ser lido
            if (dump_blk(fs, indirect[i], to_read) < 0)                                       // Lê o bloco indireto
                return -1;
            bytes_left -= to_read;
        }
    }

    // Bloco indireto duplo
    if (bytes_left > 0 && in->i_block[13])
    {
        uint8_t buf1[EXT2_BLOCK_SIZE], buf2[EXT2_BLOCK_SIZE]; // Buffers para leitura
        if (fs_read_block(fs, in->i_block[13], buf1) < 0)
            return -1;
        uint32_t *dbl_indirect = (uint32_t *)buf1;

        for (uint32_t i = 0; i < PTRS_PER_BLOCK && bytes_left > 0; ++i) // Lê os blocos indiretos duplos
        {
            if (!dbl_indirect[i]) // Verifica se o ponteiro é nulo
                continue;
            if (fs_read_block(fs, dbl_indirect[i], buf2) < 0) // Lê o bloco indireto duplo
                return -1;
            uint32_t *indirect = (uint32_t *)buf2;
            for (uint32_t j = 0; j < PTRS_PER_BLOCK && bytes_left > 0; ++j) // Lê os blocos indiretos simples dentro do indireto duplo
            {
                uint32_t to_read = (bytes_left > EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : bytes_left; // Tamanho a ser lido
                // Lê o bloco indireto simples
                if (dump_blk(fs, indirect[j], to_read) < 0)
                    return -1;
                bytes_left -= to_read; // Atualiza o número de bytes restantes
            }
        }
    }

    return 0;
}

int cmd_cat(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    char *abs = fs_join_path(fs, *cwd, argv[1]); // Resolve o caminho absoluto do arquivo
    if (!abs)
        return -1;

    uint32_t ino;
    if (fs_path_resolve(fs, abs, &ino) < 0) // Resolve o inode do arquivo
    {
        free(abs);
        return -1;
    }

    struct ext2_inode in;
    if (fs_read_inode(fs, ino, &in) < 0) // Lê o inode do arquivo
    {
        free(abs);
        return -1;
    }
    free(abs);

    if (!ext2_is_reg(&in)) // Verifica se o inode é um arquivo regular
    {
        errno = EISDIR;
        fprintf(stderr, "%s: não é arquivo regular\n", argv[1]);
        return -1;
    }

    if (dump_file(fs, &in) < 0) // Lê o conteúdo do arquivo e escreve no stdout
        return -1;
    putchar('\n');
    return 0;
}

static int find_entry_by_ino(ext2_fs_t *fs, struct ext2_inode *dir_inode, uint32_t tgt_ino, struct ext2_dir_entry *out)
{
    uint8_t buf[EXT2_BLOCK_SIZE];

    for (int i = 0; i < 12; ++i) // Percorre os blocos diretos do diretório
    {
        uint32_t bloco = dir_inode->i_block[i];
        if (!bloco)
            continue;

        if (fs_read_block(fs, bloco, buf) < 0)
            return -1;

        uint32_t offset = 0;
        while (offset < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + offset);
            if (entry->rec_len == 0)
                break;

            if (entry->inode == tgt_ino)
            {
                memcpy(out, entry, sizeof(struct ext2_dir_entry));
                size_t name_len = entry->name_len < 255 ? entry->name_len : 255; // Copia o nome separadamente, respeitando o tamanho
                memcpy(out->name, entry->name, name_len);
                out->name[name_len] = '\0'; // Garante terminação
                return 0;
            }
            offset += entry->rec_len;
        }
    }
    errno = ENOENT;
    return -1;
}

int cmd_cd(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    // Resolve o caminho absoluto do diretório alvo
    char *abs_path = fs_join_path(fs, *cwd, argv[1]);
    if (!abs_path)
        return -1;

    uint32_t dir_ino;
    if (fs_path_resolve(fs, abs_path, &dir_ino) < 0)
    {
        fprintf(stderr, "cd: diretório '%s' não encontrado\n", argv[1]);
        free(abs_path);
        return -1;
    }

    struct ext2_inode dir_inode;
    if (fs_read_inode(fs, dir_ino, &dir_inode) < 0)
    {
        fprintf(stderr, "cd: erro ao ler inode do diretório\n");
        free(abs_path);
        return -1;
    }

    if (!ext2_is_dir(&dir_inode))
    {
        fprintf(stderr, "cd: '%s' não é um diretório\n", argv[1]);
        free(abs_path);
        errno = ENOTDIR;
        return -1;
    }

    // Novo trecho: buscar o nome no diretório pai
    char *parent_path = strdup(abs_path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash && last_slash != parent_path)
    {
        *last_slash = '\0';
    }
    else
    {
        strcpy(parent_path, "/");
    }

    uint32_t parent_ino;
    if (fs_path_resolve(fs, parent_path, &parent_ino) == 0)
    {
        struct ext2_inode parent_inode;
        if (fs_read_inode(fs, parent_ino, &parent_inode) == 0)
        {
            struct ext2_dir_entry entry;
            if (find_entry_by_ino(fs, &parent_inode, dir_ino, &entry) == 0)
            {
                print_entry(&entry); // Agora imprime o nome correto
            }
        }
    }
    free(parent_path);

    // Atualiza o diretório corrente
    *cwd = dir_ino;

    free(abs_path);
    return 0;
}

int dump_block(ext2_fs_t *fs, uint32_t blk, FILE *fd, uint32_t bytes)
{
    static unsigned char zero_block[EXT2_BLOCK_SIZE] = {0}; // Bloco de zeros para blocos não alocados
    unsigned char data_block[EXT2_BLOCK_SIZE];              // Buffer para armazenar os dados do bloco lido
    const void *data = zero_block;                          // Inicializa com bloco de zeros

    if (blk != 0) // Se o bloco não for zero (não alocado)
    {
        if (fs_read_block(fs, blk, data_block) < 0) // Lê o bloco do sistema de arquivos
        {
            printf("error");
            return EXIT_FAILURE;
        }
        data = data_block; // Usa o bloco lido como dados
    }
    size_t written = fwrite(data, 1, bytes, fd);             // Escreve os dados no arquivo de destino
    return (written == bytes) ? EXIT_SUCCESS : EXIT_FAILURE; // Retorna 0 se a escrita foi bem-sucedida, 1 se houve erro
}

int copy_ext2_to_host(ext2_fs_t *fs, uint32_t ino, const char *dst)
{
    struct ext2_inode in;                // Cria inode do arquivo
    if (fs_read_inode(fs, ino, &in) < 0) // Lê o inode do arquivo
    {
        printf("unknown error");
        return EXIT_FAILURE;
    }

    if (!ext2_is_reg(&in)) // Verifica se é um arquivo regular
    {
        printf("file not found");
        return EXIT_FAILURE;
    }

    FILE *fd = fopen(dst, "wb"); // Abre o arquivo de destino para escrita binária
    if (!fd)                     // Verifica se o arquivo de destino existe e foi aberto corretamente
    {
        printf("destination directory not exists");
        return EXIT_FAILURE;
    }
    uint32_t bytes_left = in.i_size; // Tamanho do arquivo a ser copiado
    int result = 0;                  // Variável para armazenar o resultado da cópia

    for (int i = 0; i < 12 && bytes_left; ++i) // Blocos diretos (0-11)
    {
        uint32_t n = (bytes_left >= EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : bytes_left; // Tamanho do bloco a ser copiado
        if (dump_block(fs, in.i_block[i], fd, n))                                    // Copia o bloco para o arquivo de destino
        {
            printf("unknown error");
            result = EXIT_FAILURE;
            break;
        }
        bytes_left -= n;
    }

    if (result == 0 && bytes_left) // Bloco indireto simples (12)
    {
        unsigned char buf[EXT2_BLOCK_SIZE] = {0}; // Buffer para armazenar os blocos indiretos
        if (in.i_block[12])                       // Verifica se o bloco indireto simples está alocado
        {
            if (fs_read_block(fs, in.i_block[12], buf) < 0)
            {
                printf("unknown error");
                result = EXIT_FAILURE;
            }
        }
        if (result == 0) // Se não houve erro ao ler o bloco indireto simples
        {
            uint32_t *tbl = (uint32_t *)buf;                            // Converte o buffer para um ponteiro de tabela de blocos
            for (uint32_t j = 0; j < PTRS_PER_BLOCK && bytes_left; ++j) // Percorre os ponteiros na tabela de blocos indiretos
            {
                uint32_t n = (bytes_left >= EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : bytes_left; // Tamanho do bloco a ser copiado
                uint32_t blk = in.i_block[12] ? tbl[j] : 0;                                  // Obtém o bloco a ser copiado

                if (dump_block(fs, blk, fd, n)) // Copia o bloco para o arquivo de destino
                {
                    printf("unknown error");
                    result = EXIT_FAILURE;
                    break;
                }
                bytes_left -= n; // Atualiza o tamanho restante do arquivo a ser copiado
            }
        }
    }

    if (result == 0 && bytes_left) // Bloco indireto duplo (13)
    {
        if (!in.i_block[13]) // Se não houver bloco indireto duplo, não há mais blocos a serem copiados
        {
            printf("unknown error");
            result = EXIT_FAILURE;
        }
        else // Se houver bloco indireto duplo, lê os blocos indiretos
        {
            unsigned char buf1[EXT2_BLOCK_SIZE], buf2[EXT2_BLOCK_SIZE]; // Buffers para armazenar os blocos indiretos
            if (fs_read_block(fs, in.i_block[13], buf1) < 0)            // Lê o bloco indireto duplo
            {
                printf("unknown error");
                result = EXIT_FAILURE;
            }
            else // Se não houve erro ao ler o bloco indireto duplo
            {
                uint32_t *lvl1 = (uint32_t *)buf1; // Converte o buffer para um ponteiro de tabela de blocos indiretos duplos

                for (uint32_t i1 = 0; i1 < PTRS_PER_BLOCK && bytes_left && result == 0; ++i1) // Percorre os ponteiros na tabela de blocos indiretos duplos
                {
                    uint32_t l2blk = lvl1[i1];
                    if (l2blk && fs_read_block(fs, l2blk, buf2) < 0)
                    {
                        printf("unknown error");
                        result = EXIT_FAILURE;
                        break;
                    }
                    uint32_t *lvl2 = (uint32_t *)buf2;
                    for (uint32_t i2 = 0; i2 < PTRS_PER_BLOCK && bytes_left && result == 0; ++i2)
                    {
                        uint32_t blk = l2blk ? lvl2[i2] : 0;
                        uint32_t n = (bytes_left >= EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : bytes_left;

                        if (dump_block(fs, blk, fd, n)) // Copia o bloco para o arquivo de destino
                        {
                            printf("unknown error");
                            result = EXIT_FAILURE;
                            break;
                        }
                        bytes_left -= n; // Atualiza o tamanho restante do arquivo a ser copiado
                    }
                }
            }
        }
    }

    fclose(fd);
    if (result) // Se houve erro durante a cópia, remove o arquivo de destino
        remove(dst);
    return result; // Retorna 0 se a cópia foi bem-sucedida, 1 se houve erro
}

static int resolve_image_path(ext2_fs_t *fs, uint32_t cwd, char *arg, char **abs_out, uint32_t *ino_out)
{
    // Declara um ponteiro para armazenar o caminho absoluto
    char *abs_path;

    if (arg[0] == '/')          // Se o argumento já começa com '/', é um caminho absoluto
        abs_path = strdup(arg); // Faz uma cópia do caminho absoluto
    else                        // Caso contrário, constrói o caminho absoluto a partir do diretório atual (cwd)
        abs_path = fs_join_path(fs, cwd, arg);

    if (!abs_path) // Se não foi possível obter o caminho absoluto, retorna erro
    {
        printf("unknown error");
        return EXIT_FAILURE;
    }

    int found = fs_path_resolve(fs, abs_path, ino_out); // Resolve o caminho absoluto para obter o número do inode correspondente
    *abs_out = abs_path;                                // Retorna o caminho absoluto pelo ponteiro de saída

    return found == 0 ? EXIT_SUCCESS : EXIT_FAILURE; // Retorna EXIT_SUCCESS se encontrou, EXIT_FAILURE caso contrário
}

void make_dst_path(char *dst_full, const char *dst, const char *src_path)
{
    size_t len = strlen(dst); // Obtém o comprimento da string de destino

    if (dst[len - 1] != '/') // Se o destino NÃO termina com '/'...
    {
        const char *last_slash = strrchr(dst, '/'); // Procura a última barra '/' no destino
        const char *last_dot = strrchr(dst, '.');   // Procura o último ponto '.' no destino

        if (!last_dot || (last_slash && last_dot < last_slash)) // Se NÃO há ponto após a última barra, considera como diretório
        {
            snprintf(dst_full, 4096, "%s/", dst); // Adiciona '/' ao final do destino e armazena em dst_full
            dst = dst_full;                       // Atualiza dst para apontar para dst_full (agora com '/')
            len = strlen(dst);                    // Atualiza o comprimento do novo destino
        }
    }

    if (dst[len - 1] == '/') // Se o destino termina com '/', é um diretório
    {

        const char *base = strrchr(src_path, '/'); // Obtém o nome do arquivo base de src_path (após a última '/')
        base = base ? base + 1 : src_path;         // Se base for NULL, usa src_path inteiro como base
        strcpy(dst_full, dst);                     // Copia o destino (com '/') para dst_full
        strcat(dst_full, base);                    // Adiciona o nome do arquivo base ao destino
    }
    else // Caso contrário, apenas copia o destino para dst_full
    {
        strcpy(dst_full, dst);
    }
}

int cmd_cp(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 3) // Verifica se o número de argumentos é válido
    {
        printf("invalid syntax");
        return EXIT_FAILURE;
    }

    if (argv[2][0] != '/') // Destino deve ser caminho absoluto do sistema real
    {
        printf("destination directory not exists");
        return EXIT_FAILURE;
    }

    char *src_path = NULL;                                          // Caminho absoluto do arquivo de origem
    uint32_t src_ino = 0;                                           // Inode do arquivo de origem
    if (resolve_image_path(fs, *cwd, argv[1], &src_path, &src_ino)) // Resolve o caminho do arquivo de origem
    {
        printf("file not found");
        free(src_path);
        return EXIT_FAILURE;
    }

    char dst_full[4096];                        // Caminho completo do destino
    make_dst_path(dst_full, argv[2], src_path); // Cria o caminho completo do destino

    FILE *dst_file = fopen(dst_full, "rb");
    if (dst_file != NULL) // Verifica se o arquivo de destino já existe
    {
        fclose(dst_file);
        printf("file or directory already exists");
        free(src_path);
        return EXIT_FAILURE;
    }

    int res = copy_ext2_to_host(fs, src_ino, dst_full); // Copia o arquivo do EXT2 para o sistema real

    free(src_path);

    return res ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int list_directory(ext2_fs_t *fs, struct ext2_inode *dir_inode)
{
    uint8_t buf[EXT2_BLOCK_SIZE];

    // Percorre os blocos diretos do inode do diretório
    for (int i = 0; i < 12; ++i)
    {
        uint32_t bloco = dir_inode->i_block[i];
        if (bloco == 0)
            continue;

        if (fs_read_block(fs, bloco, buf) < 0)
            return -1;

        uint32_t offset = 0;
        while (offset < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + offset);
            if (entry->rec_len == 0)
                break; // Bloco corrompido

            if (entry->inode != 0)
            {
                print_entry(entry);
            }
            offset += entry->rec_len;
        }
    }
    return 0;
}

int cmd_ls(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc > 2)
    {
        fprintf(stderr, "invalid syntax\n");
        errno = EINVAL;
        return -1;
    }

    uint32_t alvo_inode;
    char *caminho_completo = NULL;

    // Se nenhum argumento, lista o diretório atual
    if (argc == 1)
    {
        alvo_inode = *cwd;
    }
    else
    {
        caminho_completo = fs_join_path(fs, *cwd, argv[1]);
        if (!caminho_completo)
            return -1;
        if (fs_path_resolve(fs, caminho_completo, &alvo_inode) < 0)
        {
            free(caminho_completo);
            return -1;
        }
    }

    struct ext2_inode inode;
    if (fs_read_inode(fs, alvo_inode, &inode) < 0)
    {
        free(caminho_completo);
        return -1;
    }

    int resultado;
    if (ext2_is_dir(&inode))
    {
        resultado = list_directory(fs, &inode);
    }
    else
    {
        struct ext2_dir_entry entrada_fake = {
            .inode = alvo_inode,
            .rec_len = inode.i_blocks ? inode.i_blocks * 512 : 0,
            .name_len = (uint8_t)strlen(argv[argc == 1 ? 0 : 1]),
            .file_type = EXT2_FT_REG_FILE};
        strncpy((char *)entrada_fake.name, argc == 1 ? "(arquivo)" : argv[1], entrada_fake.name_len);
        print_entry(&entrada_fake);
        resultado = 0;
    }

    free(caminho_completo);
    return resultado;
}

static int dir_add_entry(ext2_fs_t *fs, struct ext2_inode *dir_inode, uint32_t dir_ino, uint32_t new_ino, char *name, uint8_t file_type)
{
    uint8_t buf[EXT2_BLOCK_SIZE];
    uint16_t tamanho_necessario = rec_len_needed((uint8_t)strlen(name));

    // Percorre os blocos diretos do diretório
    for (int i = 0; i < 12; ++i)
    {
        uint32_t bloco = dir_inode->i_block[i];

        // Se o bloco não existe, aloca um novo bloco para o diretório
        if (!bloco)
        {
            if (fs_alloc_block(fs, &bloco) < 0)
                return -1;
            dir_inode->i_block[i] = bloco;
            dir_inode->i_size += EXT2_BLOCK_SIZE;
            dir_inode->i_blocks += EXT2_BLOCK_SIZE / 512;

            // Cria a nova entrada ocupando todo o bloco
            memset(buf, 0, EXT2_BLOCK_SIZE);
            struct ext2_dir_entry *entrada = (struct ext2_dir_entry *)buf; // Inicia a entrada de diretório
            entrada->inode = new_ino;
            entrada->rec_len = EXT2_BLOCK_SIZE;
            entrada->name_len = (uint8_t)strlen(name);
            entrada->file_type = file_type;
            memcpy(entrada->name, name, entrada->name_len); // Copia o nome para a entrada

            if (fs_write_block(fs, bloco, buf) < 0) // Escreve o bloco no disco
                return -1;
            if (fs_write_inode(fs, dir_ino, dir_inode) < 0) // Atualiza o inode do diretório
                return -1;
            return 0;
        }

        if (fs_read_block(fs, bloco, buf) < 0) // Lê o bloco existente
            return -1;

        uint32_t pos = 0;
        // Percorre as entradas do bloco procurando espaço livre
        while (pos < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entrada = (struct ext2_dir_entry *)(buf + pos); // Obtém a entrada de diretório atual
            if (entrada->rec_len == 0)
                break;

            uint16_t ideal = rec_len_needed(entrada->name_len);
            uint16_t espaco_livre = entrada->rec_len - ideal;

            // Se houver espaço suficiente após esta entrada, insere a nova entrada aqui
            if (espaco_livre >= tamanho_necessario)
            {
                entrada->rec_len = ideal;
                struct ext2_dir_entry *nova = (struct ext2_dir_entry *)(buf + pos + ideal); // Nova entrada de diretório
                nova->inode = new_ino;
                nova->rec_len = espaco_livre;
                nova->name_len = (uint8_t)strlen(name);
                nova->file_type = file_type;
                memcpy(nova->name, name, nova->name_len); // Copia o nome para a nova entrada

                if (fs_write_block(fs, bloco, buf) < 0) // Escreve o bloco no disco
                    return -1;
                if (fs_write_inode(fs, dir_ino, dir_inode) < 0) // Atualiza o inode do diretório
                    return -1;
                return 0;
            }
            pos += entrada->rec_len; // Avança para a próxima entrada
        }
    }

    // Não há espaço disponível nos blocos diretos
    errno = ENOSPC;
    return -1;
}

int cmd_mkdir(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    // Monta o caminho absoluto do novo diretório
    char *novo_caminho = fs_join_path(fs, *cwd, argv[1]);
    if (!novo_caminho)
        return -1;

    // Verifica se já existe um arquivo ou diretório com esse nome
    uint32_t inode_existente;
    if (fs_path_resolve(fs, novo_caminho, &inode_existente) == 0)
    {
        fprintf(stderr, "mkdir: '%s' já existe\n", argv[1]);
        free(novo_caminho);
        errno = EEXIST;
        return -1;
    }

    // Separa o caminho do diretório pai e o nome do novo diretório
    char *ultimo_slash = strrchr(novo_caminho, '/');
    char *nome_dir = ultimo_slash ? ultimo_slash + 1 : novo_caminho;
    char *caminho_pai;
    if (ultimo_slash)
    {
        if (ultimo_slash == novo_caminho)
            caminho_pai = strdup("/");
        else
        {
            *ultimo_slash = '\0';
            caminho_pai = strdup(novo_caminho);
            *ultimo_slash = '/';
        }
    }
    else
    {
        caminho_pai = fs_get_path(fs, *cwd);
    }
    if (!caminho_pai)
    {
        free(novo_caminho);
        return -1;
    }

    // Resolve o inode do diretório pai
    uint32_t inode_pai;
    if (fs_path_resolve(fs, caminho_pai, &inode_pai) < 0) // Resolve o inode do diretório pai
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    struct ext2_inode inode_pai_struct;
    if (fs_read_inode(fs, inode_pai, &inode_pai_struct) < 0) // Lê o inode do diretório pai
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }
    if (!ext2_is_dir(&inode_pai_struct)) // Verifica se o inode do pai é um diretório
    {
        fprintf(stderr, "mkdir: '%s' não é um diretório\n", caminho_pai);
        errno = ENOTDIR;
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    // Aloca inode para o novo diretório
    uint32_t novo_inode;
    if (fs_alloc_inode(fs, EXT2_S_IFDIR | 0755, &novo_inode) < 0) // Aloca um novo inode para o diretório
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    // Aloca o primeiro bloco do novo diretório
    uint32_t novo_bloco;
    if (fs_alloc_block(fs, &novo_bloco) < 0) // Aloca um novo bloco para o diretório
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    // Prepara o inode do novo diretório
    struct ext2_inode novo_inode_struct = {0};
    novo_inode_struct.i_mode = EXT2_S_IFDIR | 0755;
    novo_inode_struct.i_uid = 0;
    novo_inode_struct.i_gid = 0;
    novo_inode_struct.i_size = EXT2_BLOCK_SIZE;
    novo_inode_struct.i_blocks = EXT2_BLOCK_SIZE / 512;
    novo_inode_struct.i_links_count = 2; // '.' e '..'
    time_t agora = time(NULL);
    novo_inode_struct.i_atime = novo_inode_struct.i_ctime = novo_inode_struct.i_mtime = (uint32_t)agora; // Define os tempos de acesso, criação e modificação
    novo_inode_struct.i_block[0] = novo_bloco;

    if (fs_write_inode(fs, novo_inode, &novo_inode_struct) < 0) // Escreve o inode do novo diretório no disco
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    // Cria as entradas '.' e '..' no novo diretório
    uint8_t buffer[EXT2_BLOCK_SIZE];
    memset(buffer, 0, EXT2_BLOCK_SIZE);
    struct ext2_dir_entry *ponto = (struct ext2_dir_entry *)buffer; // Posição da entrada '.'
    ponto->inode = novo_inode;
    ponto->name_len = 1;
    ponto->file_type = EXT2_FT_DIR;
    ponto->rec_len = rec_len_needed(1);
    ponto->name[0] = '.';

    struct ext2_dir_entry *ponto_ponto = (struct ext2_dir_entry *)(buffer + ponto->rec_len); // Posição da entrada '..'
    ponto_ponto->inode = inode_pai;
    ponto_ponto->name_len = 2;
    ponto_ponto->file_type = EXT2_FT_DIR;
    ponto_ponto->rec_len = EXT2_BLOCK_SIZE - ponto->rec_len;
    ponto_ponto->name[0] = '.';
    ponto_ponto->name[1] = '.';

    if (fs_write_block(fs, novo_bloco, buffer) < 0) // Escreve o bloco do novo diretório no disco
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    if (dir_add_entry(fs, &inode_pai_struct, inode_pai, novo_inode, nome_dir, EXT2_FT_DIR) < 0) // Adiciona a entrada do novo diretório no diretório pai
    {
        free(novo_caminho);
        free(caminho_pai);
        return -1;
    }

    // Atualiza o link count do diretório pai
    inode_pai_struct.i_links_count++;
    fs_write_inode(fs, inode_pai, &inode_pai_struct);

    printf("Diretório criado: %s (inode: %u)\n", argv[1], novo_inode);

    free(novo_caminho);
    free(caminho_pai);
    return 0;
}

static void print_super(struct ext2_super_block *sb)
{
    printf("inodes count..................: %u\n", sb->s_inodes_count);
    printf("blocks count..................: %u\n", sb->s_blocks_count);
    printf("reserved blocks count.........: %u\n", sb->s_r_blocks_count);
    printf("free blocks count.............: %u\n", sb->s_free_blocks_count);
    printf("free inodes count.............: %u\n", sb->s_free_inodes_count);
    printf("first data block..............: %u\n", sb->s_first_data_block);
    printf("block size....................: %u\n", 1024 << sb->s_log_block_size);
    printf("fragment size.................: %u\n", 1024 << sb->s_log_frag_size);
    printf("blocks per group..............: %u\n", sb->s_blocks_per_group);
    printf("fragments per group...........: %u\n", sb->s_frags_per_group);
    printf("inodes per group..............: %u\n", sb->s_inodes_per_group);
    printf("mount time....................: %u\n", sb->s_mtime);
    printf("write time....................: %u\n", sb->s_wtime);
    printf("mount count...................: %u\n", sb->s_mnt_count);
    printf("max mount count...............: %u\n", sb->s_max_mnt_count);
    printf("magic signature...............: 0x%x\n", sb->s_magic);
    printf("file system state.............: %u\n", sb->s_state);
    printf("errors........................: %u\n", sb->s_errors);
    printf("minor revision level..........: %u\n", sb->s_minor_rev_level);
    time_t last = sb->s_lastcheck;
    char buf[32];
    strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", localtime(&last));
    printf("time of last check............: %s\n", buf);
    printf("max check interval............: %u\n", sb->s_checkinterval);
    printf("creator OS....................: %u\n", sb->s_creator_os);
    printf("revision level................: %u\n", sb->s_rev_level);
    printf("default uid reserved blocks...: %u\n", sb->s_def_resuid);
    printf("default gid reserved blocks...: %u\n", sb->s_def_resgid);
    printf("first non-reserved inode......: %u\n", sb->s_first_ino);
    printf("inode size....................: %u\n", sb->s_inode_size);
    printf("block group number............: %u\n", sb->s_block_group_nr);
    printf("compatible feature set........: %u\n", sb->s_feature_compat);
    printf("incompatible feature set......: %u\n", sb->s_feature_incompat);
    printf("read only comp feature set....: %u\n", sb->s_feature_ro_compat);
    printf("volume UUID...................: ");
    for (int i = 0; i < 16; ++i)
    {
        printf("%02x", sb->s_uuid[i]);
    }
    printf("\n");
    printf("volume name...................: %.*s\n", 16, sb->s_volume_name);
    printf("volume last mounted...........: %.*s\n", 64, sb->s_last_mounted);
    printf("algorithm usage bitmap........: %u\n", sb->s_algo_bitmap);
    printf("blocks to try to preallocate..: %u\n", sb->s_prealloc_blocks);
    printf("blocks preallocate dir........: %u\n", sb->s_prealloc_dir_blocks);
    printf("journal UUID..................: ");
    for (int i = 0; i < 16; ++i)
    {
        printf("%02x", sb->s_journal_uuid[i]);
    }
    printf("\n");
    printf("journal INum..................: %u\n", sb->s_journal_inum);
    printf("journal Dev...................: %u\n", sb->s_journal_dev);
    printf("last orphan...................: %u\n", sb->s_last_orphan);
    printf("hash seed.....................: ");
    for (int i = 0; i < 4; ++i)
    {
        printf("%08x", sb->s_hash_seed[i]);
    }
    printf("\n");
    printf("default hash version..........: %u\n", sb->s_def_hash_version);
    printf("default mount options.........: %u\n", sb->s_default_mount_options);
    printf("first meta....................: %u\n", sb->s_first_meta_bg);
}

static void print_groups(ext2_fs_t *fs)
{
    struct ext2_group_desc gd;
    for (uint32_t g = 0; g < fs->groups_count; ++g)
    {
        if (fs_read_group_desc(fs, g, &gd) < 0)
            break;
        printf("Block Group Descriptor %u:\n", g);
        printf("    block bitmap.............: %u\n", gd.bg_block_bitmap);
        printf("    inode bitmap.............: %u\n", gd.bg_inode_bitmap);
        printf("    inode table..............: %u\n", gd.bg_inode_table);
        printf("    free blocks count........: %u\n", gd.bg_free_blocks_count);
        printf("    free inodes count........: %u\n", gd.bg_free_inodes_count);
        printf("    used dirs count..........: %u\n", gd.bg_used_dirs_count);
    }
}

static void print_inode(ext2_fs_t *fs, uint32_t ino)
{
    struct ext2_inode in;
    if (fs_read_inode(fs, ino, &in) < 0)
    {
        perror("inode");
        return;
    }
    printf("file format and access rights..: 0x%x\n", in.i_mode);
    printf("user id........................: %u\n", in.i_uid);
    printf("lower 32-bit file size.........: %u\n", in.i_size);
    printf("access time....................: %u\n", in.i_atime);
    printf("creation time..................: %u\n", in.i_ctime);
    printf("modification time..............: %u\n", in.i_mtime);
    printf("deletion time..................: %u\n", in.i_dtime);
    printf("group id.......................: %u\n", in.i_gid);
    printf("link count inode...............: %u\n", in.i_links_count);
    printf("512-bytes blocks...............: %u\n", in.i_blocks);
    printf("ext2 flags.....................: %u\n", in.i_flags);
    printf("reserved (Linux)...............: %u\n", in.i_osd1);
    for (int i = 0; i < 15; ++i)
        printf("pointer[%2d].....................: %u\n", i, in.i_block[i]);
    printf("file version (nfs).............: %u\n", in.i_generation);
    printf("block number ext. attributes...: %u\n", in.i_file_acl);
    printf("higher 32-bit file size........: %u\n", in.i_dir_acl);
    printf("location file fragment.........: %u\n", in.i_faddr);
}

int cmd_print(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    (void)cwd;
    if (argc < 2)
    {
        return -1;
    }
    if (strcmp(argv[1], "superblock") == 0)
    {
        print_super(&fs->sb);
        return 0;
    }
    if (strcmp(argv[1], "groups") == 0)
    {
        print_groups(fs);
        return 0;
    }
    if (strcmp(argv[1], "inode") == 0)
    {
        if (argc != 3)
        {
            return -1;
        }
        uint32_t ino = strtoul(argv[2], NULL, 0);
        print_inode(fs, ino);
        return 0;
    }
    fprintf(stderr, "print: parâmetro não reconhecido\n");
    return -1;
}

int cmd_pwd(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    // Ignora argumentos além do comando
    (void)argv;

    // Verifica se o comando foi chamado corretamente
    if (argc != 1)
    {
        errno = EINVAL;
        return -1;
    }

    // Obtém o caminho absoluto do diretório atual
    char *current_path = fs_get_path(fs, *cwd);
    if (!current_path)
    {
        fprintf(stderr, "Erro ao obter o caminho do diretório atual.\n");
        return -1;
    }

    // Imprime o caminho
    printf("%s\n", current_path);

    // Libera a memória alocada para o caminho
    free(current_path);
    return 0;
}

static int rename_entry_block(ext2_fs_t *fs, uint32_t blk, uint32_t target_ino, char *newname)
{
    uint8_t buf[EXT2_BLOCK_SIZE];
    if (fs_read_block(fs, blk, buf) < 0) // Lê o bloco do diretório
        return -1;

    uint32_t pos = 0;
    while (pos < EXT2_BLOCK_SIZE)
    {
        struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + pos); // Obtém a entrada de diretório atual
        if (entry->rec_len == 0)
            break;

        if (entry->inode == target_ino)
        {
            uint8_t newname_len = (uint8_t)strlen(newname);      // Comprimento do novo nome
            uint16_t required_len = rec_len_needed(newname_len); // Calcula o comprimento necessário para a nova entrada

            if (required_len > entry->rec_len)
            {
                errno = ENOSPC;
                return -1;
            }

            entry->name_len = newname_len;
            memcpy(entry->name, newname, newname_len); // Copia o novo nome para a entrada

            // Preenche com zeros o restante do nome antigo, se necessário
            if (newname_len < entry->rec_len - 8)
                memset(entry->name + newname_len, 0, entry->rec_len - 8 - newname_len); // Preenche com zeros após o novo nome

            // Salva o bloco modificado
            return fs_write_block(fs, blk, buf);
        }

        pos += entry->rec_len; // Avança para a próxima entrada
    }

    errno = ENOENT;
    return -1;
}

int cmd_rename(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 3)
    {
        errno = EINVAL;
        return -1;
    }

    char *old_path = argv[1];
    char *new_name = argv[2];

    if (strlen(new_name) > 255)
    {
        fprintf(stderr, "novo_nome muito longo\n");
        errno = ENAMETOOLONG;
        return -1;
    }

    // Resolve o caminho completo do arquivo antigo
    char *old_full_path = fs_join_path(fs, *cwd, old_path);
    if (!old_full_path)
        return -1;

    uint32_t old_inode;
    if (fs_path_resolve(fs, old_full_path, &old_inode) < 0) // Resolve o inode do arquivo antigo
    {
        free(old_full_path);
        return -1;
    }

    // Descobre o diretório pai do arquivo
    char *parent_path;
    char *dup_path = strdup(old_full_path);
    char *last_slash = strrchr(dup_path, '/');
    if (!last_slash)
        parent_path = fs_get_path(fs, *cwd);
    else if (last_slash == dup_path)
        parent_path = strdup("/");
    else
    {
        *last_slash = '\0';
        parent_path = strdup(dup_path);
    }

    uint32_t parent_inode_num;
    if (fs_path_resolve(fs, parent_path, &parent_inode_num) < 0) // Resolve o inode do diretório pai
    {
        free(old_full_path);
        free(dup_path);
        free(parent_path);
        return -1;
    }

    struct ext2_inode parent_inode;
    if (fs_read_inode(fs, parent_inode_num, &parent_inode) < 0) // Lê o inode do diretório pai
    {
        free(old_full_path);
        free(dup_path);
        free(parent_path);
        return -1;
    }

    int already_exists = name_exists(fs, &parent_inode, new_name); // Verifica se já existe uma entrada com o novo nome
    if (already_exists < 0)
    {
        free(old_full_path);
        free(dup_path);
        free(parent_path);
        return -1;
    }
    if (already_exists == 1)
    {
        fprintf(stderr, "%s: já existe\n", new_name);
        free(old_full_path);
        free(dup_path);
        free(parent_path);
        errno = EEXIST;
        return -1;
    }

    // Procura e renomeia a entrada no diretório pai
    int renamed = 0;
    for (int i = 0; i < 12; ++i)
    {
        uint32_t block = parent_inode.i_block[i];
        if (!block)
            continue;
        if (rename_entry_block(fs, block, old_inode, new_name) == 0) // Renomeia a entrada no bloco
        {
            renamed = 1;
            break;
        }
        if (errno != ENOENT)
        {
            free(old_full_path);
            free(dup_path);
            free(parent_path);
            return -1;
        }
    }
    if (!renamed)
    {
        free(old_full_path);
        free(dup_path);
        free(parent_path);
        errno = ENOENT;
        return -1;
    }

    free(old_full_path);
    free(dup_path);
    free(parent_path);
    return 0;
}

int fs_free_blocks(ext2_fs_t *fs, uint32_t blk)
{
    struct ext2_group_desc gd;
    uint8_t bitmap[EXT2_BLOCK_SIZE];

    /* Grupo ao qual o bloco pertence */
    uint32_t group = (blk - fs->sb.s_first_data_block) / fs->sb.s_blocks_per_group;

    if (fs_read_group_desc(fs, group, &gd) < 0)
        return -1;
    if (fs_read_block(fs, gd.bg_block_bitmap, bitmap) < 0)
        return -1;

    uint32_t idx = (blk - fs->sb.s_first_data_block) % fs->sb.s_blocks_per_group;

    /* bloco já livre → inconsistência */
    if (!(bitmap[BIT_BYTE(idx)] & BIT_MASK(idx)))
    {
        errno = EINVAL;
        return -1;
    }

    bitmap[BIT_BYTE(idx)] &= ~BIT_MASK(idx); /* marca como livre */
    gd.bg_free_blocks_count++;
    fs->sb.s_free_blocks_count++;

    if (fs_write_block(fs, gd.bg_block_bitmap, bitmap) < 0)
        return -1;
    if (fs_write_group_desc(fs, group, &gd) < 0)
        return -1;

    return fs_sync_super(fs);
}

int free_indirect_chain(ext2_fs_t *fs, uint32_t blk, int depth)
{
    if (!blk)
        return 0;

    uint32_t ptrs[PTRS_PER_BLOCK]; // Ponteiros para blocos
    if (fs_read_block(fs, blk, ptrs) < 0)
        return -1;

    if (depth == 1)
    {
        for (size_t i = 0; i < PTRS_PER_BLOCK; ++i)         // Percorre os ponteiros
            if (ptrs[i] && fs_free_blocks(fs, ptrs[i]) < 0) // Libera blocos diretos
                return -1;
    }
    else
    {
        for (size_t i = 0; i < PTRS_PER_BLOCK; ++i)
            if (ptrs[i] && free_indirect_chain(fs, ptrs[i], depth - 1) < 0) // Libera blocos indiretos
                return -1;
    }

    return fs_free_blocks(fs, blk); // Libera o bloco indireto em si
}

int free_inode_block(ext2_fs_t *fs, struct ext2_inode *ino)
{
    /* diretos ----------------------------------------------------------- */
    for (int i = 0; i < 12; ++i)
        if (ino->i_block[i] && fs_free_blocks(fs, ino->i_block[i]) < 0)
            return -1;

    /* indireto simples -------------------------------------------------- */
    if (free_indirect_chain(fs, ino->i_block[12], 1) < 0)
        return -1;

    /* indireto duplo ---------------------------------------------------- */
    if (free_indirect_chain(fs, ino->i_block[13], 2) < 0)
        return -1;

    /* zera ponteiros + estatísticas do inode --------------------------- */
    memset(ino->i_block, 0, sizeof(ino->i_block));
    ino->i_blocks = 0;
    ino->i_size = 0;
    ino->i_dtime = time(NULL);

    return 0;
}

int dir_remove_entry_rm(ext2_fs_t *fs, struct ext2_inode *dir_inode, uint32_t target_ino)
{
    uint8_t buf[EXT2_BLOCK_SIZE];

    for (int i = 0; i < 12; ++i)
    {
        uint32_t bloco = dir_inode->i_block[i];
        if (!bloco)
            continue;

        if (fs_read_block(fs, bloco, buf) < 0)
            return -1;

        uint32_t off = 0;
        while (off < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *e = (void *)(buf + off);
            if (!e->rec_len)
                break;

            if (e->inode == target_ino)
            { /* apaga */
                e->inode = 0;
                if (fs_write_block(fs, bloco, buf) < 0)
                    return -1;
                return 0;
            }
            off += e->rec_len;
        }
    }
    errno = ENOENT;
    return -1;
}

int cmd_rm(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    char *full_path = fs_join_path(fs, *cwd, argv[1]); // Junta o caminho atual com o nome do arquivo
    if (!full_path)
        return -1;

    uint32_t file_ino;
    if (fs_path_resolve(fs, full_path, &file_ino) < 0) // Resolve o inode do arquivo
    {
        free(full_path);
        return -1;
    }

    struct ext2_inode file_inode;
    if (fs_read_inode(fs, file_ino, &file_inode) < 0) // Lê o inode do arquivo
    {
        free(full_path);
        return -1;
    }

    if (ext2_is_dir(&file_inode)) // Verifica se é um diretório
    {
        fprintf(stderr, "%s: é um diretório (use rmdir)\n", argv[1]);
        free(full_path);
        errno = EISDIR;
        return -1;
    }

    char *parent_path = strdup(full_path);   // Duplica o caminho completo
    char *slash = strrchr(parent_path, '/'); // Encontra a última barra no caminho
    if (slash && slash != parent_path)       // Se houver uma barra e não for o início do caminho
        *slash = '\0';
    else // Se não houver barra ou for o início do caminho
        strcpy(parent_path, "/");

    uint32_t parent_ino;
    if (fs_path_resolve(fs, parent_path, &parent_ino) < 0) // Resolve o inode do diretório pai
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    struct ext2_inode parent_inode;
    if (fs_read_inode(fs, parent_ino, &parent_inode) < 0) // Lê o inode do diretório pai
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    if (dir_remove_entry_rm(fs, &parent_inode, file_ino) < 0) // Remove a entrada do diretório pai
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    if (free_inode_block(fs, &file_inode) < 0) // Libera os blocos do inode
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    if (fs_free_inode(fs, file_ino) < 0) // Libera o inode
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    free(full_path);
    free(parent_path);
    return 0;
}

int dir_is_empty(ext2_fs_t *fs, struct ext2_inode *dir_inode)
{
    uint8_t buf[EXT2_BLOCK_SIZE];
    for (int i = 0; i < 12; ++i)
    {
        uint32_t blk = dir_inode->i_block[i];
        if (!blk)
            continue;
        if (fs_read_block(fs, blk, buf) < 0)
            return -1;

        uint32_t pos = 0;
        while (pos < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + pos);
            if (entry->rec_len == 0)
                break;

            if (entry->inode != 0)
            {
                // Ignora "." e ".."
                if (!(entry->name_len == 1 && entry->name[0] == '.') &&
                    !(entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.'))
                {
                    errno = ENOTEMPTY;
                    return 1;
                }
            }
            pos += entry->rec_len;
        }
    }
    return 0;
}

int dir_remove_entry_rmdir(ext2_fs_t *fs, struct ext2_inode *dir_inode, uint32_t dir_ino, uint32_t tgt_ino)
{
    uint8_t buf[EXT2_BLOCK_SIZE];
    for (int i = 0; i < 12; ++i)
    {
        uint32_t blk = dir_inode->i_block[i];
        if (!blk)
            continue;
        if (fs_read_block(fs, blk, buf) < 0)
            return -1;

        uint32_t pos = 0;
        struct ext2_dir_entry *prev = NULL;
        while (pos < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + pos);
            if (entry->rec_len == 0)
                break;

            if (entry->inode == tgt_ino)
            {
                if (prev)
                {
                    // Remove entry by merging rec_len with previous entry
                    prev->rec_len += entry->rec_len;
                }
                else
                {
                    // First entry: just clear inode
                    entry->inode = 0;
                }

                if (fs_write_block(fs, blk, buf) < 0)
                    return -1;

                // Atualiza o link count do diretório
                dir_inode->i_links_count--;
                return fs_write_inode(fs, dir_ino, dir_inode);
            }

            prev = entry;
            pos += entry->rec_len;
        }
    }
    errno = ENOENT;
    return -1;
}

int cmd_rmdir(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    // Resolve o caminho absoluto do diretório a ser removido
    char *target_path = fs_join_path(fs, *cwd, argv[1]);
    if (!target_path)
        return -1;

    // Obtém o inode do diretório alvo
    uint32_t target_ino;
    if (fs_path_resolve(fs, target_path, &target_ino) < 0)
    {
        free(target_path);
        return -1;
    }

    struct ext2_inode target_inode;
    if (fs_read_inode(fs, target_ino, &target_inode) < 0)
    {
        free(target_path);
        return -1;
    }

    // Verifica se é realmente um diretório
    if (!ext2_is_dir(&target_inode))
    {
        fprintf(stderr, "%s: não é diretório\n", argv[1]);
        free(target_path);
        errno = ENOTDIR;
        return -1;
    }

    // Verifica se o diretório está vazio
    int empty = dir_is_empty(fs, &target_inode);
    if (empty == -1)
    {
        free(target_path);
        return -1;
    }
    if (empty == 1)
    {
        fprintf(stderr, "%s: diretório não vazio\n", argv[1]);
        free(target_path);
        return -1;
    }

    // Descobre o caminho do diretório pai
    char *parent_path = strdup(target_path);
    char *slash = strrchr(parent_path, '/');
    if (slash && slash != parent_path)
        *slash = '\0';
    else
        strcpy(parent_path, "/");

    // Obtém o inode do diretório pai
    uint32_t parent_ino;
    if (fs_path_resolve(fs, parent_path, &parent_ino) < 0)
    {
        free(target_path);
        free(parent_path);
        return -1;
    }

    struct ext2_inode parent_inode;
    if (fs_read_inode(fs, parent_ino, &parent_inode) < 0)
    {
        free(target_path);
        free(parent_path);
        return -1;
    }

    // Remove a entrada do diretório pai
    if (dir_remove_entry_rmdir(fs, &parent_inode, parent_ino, target_ino) < 0)
    {
        free(target_path);
        free(parent_path);
        return -1;
    }

    // Libera blocos e inode do diretório removido
    free_inode_blocks(fs, &target_inode);
    fs_free_inode(fs, target_ino);

    free(target_path);
    free(parent_path);
    return 0;
}

int cmd_touch(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd)
{
    if (argc != 2)
    {
        errno = EINVAL;
        return -1;
    }

    // Monta o caminho absoluto do arquivo
    char *full_path = fs_join_path(fs, *cwd, argv[1]);
    if (!full_path)
        return -1;

    // Se o arquivo já existe, não faz nada (poderia atualizar timestamps)
    uint32_t existing_inode;
    if (fs_path_resolve(fs, full_path, &existing_inode) == 0)
    {
        free(full_path);
        return 0;
    }

    // Separa caminho do diretório pai e nome do arquivo
    char *last_slash = strrchr(full_path, '/');
    char *file_name = last_slash ? last_slash + 1 : full_path;
    char *parent_path = NULL;

    if (last_slash)
    {
        if (last_slash == full_path)
            parent_path = strdup("/");
        else
        {
            *last_slash = '\0';
            parent_path = strdup(full_path);
            *last_slash = '/';
        }
    }
    else
    {
        parent_path = fs_get_path(fs, *cwd);
    }

    if (!parent_path)
    {
        free(full_path);
        return -1;
    }

    // Resolve inode do diretório pai
    uint32_t parent_inode_num;
    if (fs_path_resolve(fs, parent_path, &parent_inode_num) < 0)
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    struct ext2_inode parent_inode;
    if (fs_read_inode(fs, parent_inode_num, &parent_inode) < 0)
    {
        free(full_path);
        free(parent_path);
        return -1;
    }
    if (!ext2_is_dir(&parent_inode))
    {
        errno = ENOTDIR;
        free(full_path);
        free(parent_path);
        return -1;
    }

    // Aloca inode para o novo arquivo
    uint32_t new_inode_num;
    if (fs_alloc_inode(fs, EXT2_S_IFREG | 0644, &new_inode_num) < 0)
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    struct ext2_inode new_inode = {0};
    new_inode.i_mode = EXT2_S_IFREG | 0644;
    new_inode.i_uid = 0;
    new_inode.i_gid = 0;
    new_inode.i_size = 0;
    new_inode.i_links_count = 1;
    new_inode.i_blocks = 0;
    time_t now = time(NULL);
    new_inode.i_atime = new_inode.i_ctime = new_inode.i_mtime = (uint32_t)now;

    if (fs_write_inode(fs, new_inode_num, &new_inode) < 0)
    {
        free(full_path);
        free(parent_path);
        return -1;
    }

    // Insere entrada no diretório pai
    uint8_t buf[EXT2_BLOCK_SIZE];
    uint16_t entry_size = rec_len_needed((uint8_t)strlen(file_name));
    int inserted = 0;

    for (int i = 0; i < 12 && !inserted; ++i)
    {
        uint32_t block = parent_inode.i_block[i];
        if (!block)
        {
            // Aloca novo bloco para diretório
            if (fs_alloc_block(fs, &block) < 0)
            {
                free(full_path);
                free(parent_path);
                return -1;
            }
            parent_inode.i_block[i] = block;
            parent_inode.i_size += EXT2_BLOCK_SIZE;
            parent_inode.i_blocks += EXT2_BLOCK_SIZE / 512;
            memset(buf, 0, EXT2_BLOCK_SIZE);

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)buf;
            entry->inode = new_inode_num;
            entry->name_len = (uint8_t)strlen(file_name);
            entry->file_type = EXT2_FT_REG_FILE;
            entry->rec_len = EXT2_BLOCK_SIZE;
            memcpy(entry->name, file_name, entry->name_len);

            if (fs_write_block(fs, block, buf) < 0)
            {
                free(full_path);
                free(parent_path);
                return -1;
            }
            inserted = 1;
            break;
        }

        if (fs_read_block(fs, block, buf) < 0)
        {
            free(full_path);
            free(parent_path);
            return -1;
        }
        uint32_t pos = 0;
        while (pos < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(buf + pos);
            if (entry->rec_len == 0)
                break; // Corrupção

            uint16_t ideal = rec_len_needed(entry->name_len);
            uint16_t slack = entry->rec_len - ideal;
            if (slack >= entry_size)
            {
                // Encurta entrada existente e insere nova logo após
                entry->rec_len = ideal;
                struct ext2_dir_entry *new_entry = (struct ext2_dir_entry *)(buf + pos + ideal);
                new_entry->inode = new_inode_num;
                new_entry->rec_len = slack;
                new_entry->name_len = (uint8_t)strlen(file_name);
                new_entry->file_type = EXT2_FT_REG_FILE;
                memcpy(new_entry->name, file_name, new_entry->name_len);

                if (fs_write_block(fs, block, buf) < 0)
                {
                    free(full_path);
                    free(parent_path);
                    return -1;
                }
                inserted = 1;
                break;
            }
            pos += entry->rec_len;
        }
    }

    if (!inserted)
    {
        fprintf(stderr, "touch: diretório cheio\n");
        errno = ENOSPC;
        free(full_path);
        free(parent_path);
        return -1;
    }

    // Atualiza inode do diretório pai
    fs_write_inode(fs, parent_inode_num, &parent_inode);

    printf("Arquivo criado: %s (inode: %u)\n", file_name, new_inode_num);
    free(full_path);
    free(parent_path);
    return 0;
}