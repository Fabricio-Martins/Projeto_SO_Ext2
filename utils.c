#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/utils.h"

/*
 * ============================================================================
 * FILESYSTEM MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Abre uma imagem de sistema de arquivos EXT2 e inicializa a estrutura de controle.
 *
 * @param img_path Caminho para o arquivo de imagem do sistema de arquivos
 * @return Ponteiro para estrutura ext2_fs_t inicializada, ou NULL em caso de erro
 *
 * Esta função:
 * 1. Abre o arquivo de imagem em modo leitura/escrita
 * 2. Aloca e inicializa a estrutura de controle do filesystem
 * 3. Lê e valida o superbloco EXT2
 * 4. Calcula informações derivadas como número de grupos de blocos
 */
ext2_fs_t *fs_open(char *img_path)
{
    // Abre a imagem do sistema de arquivos para leitura e escrita
    int fd = open(img_path, O_RDWR);
    if (fd < 0)
        return NULL;

    // Aloca e inicializa a estrutura do sistema de arquivos com zeros
    ext2_fs_t *fs = calloc(1, sizeof(ext2_fs_t));
    if (!fs)
    {
        close(fd);
        return NULL;
    }
    fs->fd = fd;

    // Lê o superbloco da imagem (localizado no offset padrão EXT2)
    ssize_t sb_read = pread(fd, &fs->sb, sizeof(fs->sb), EXT2_SUPER_OFFSET);
    if (sb_read != sizeof(fs->sb))
    {
        free(fs);
        close(fd);
        return NULL;
    }

    // Verifica a assinatura mágica do EXT2 para validar o formato
    if (fs->sb.s_magic != EXT2_SUPER_MAGIC)
    {
        free(fs);
        close(fd);
        errno = EINVAL;
        return NULL;
    }

    // Calcula o número total de grupos de blocos no filesystem
    // Formula: ceil(total_blocks / blocks_per_group)
    fs->groups_count = (fs->sb.s_blocks_count + fs->sb.s_blocks_per_group - 1) / fs->sb.s_blocks_per_group;

    return fs;
}

/**
 * Fecha o sistema de arquivos e libera recursos associados.
 *
 * @param fs Ponteiro para estrutura do filesystem a ser fechada
 *
 * Esta função sincroniza o superbloco, fecha o descritor de arquivo
 * e libera a memória alocada para a estrutura de controle.
 */
void fs_close(ext2_fs_t *fs)
{
    if (!fs)
        return;

    fs_sync_super(fs); // Sincroniza alterações do superbloco para disco
    close(fs->fd);     // Fecha o descritor de arquivo
    free(fs);          // Libera a estrutura de controle
}

/*
 * ============================================================================
 * BLOCK MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Calcula o offset em bytes de um bloco específico no dispositivo.
 *
 * @param fs Estrutura do filesystem (não utilizada, mantida para compatibilidade)
 * @param block Número do bloco
 * @return Offset em bytes do início do bloco no dispositivo
 */
off_t fs_block_offset(ext2_fs_t *fs, uint32_t block)
{
    (void)fs; // Parâmetro não utilizado
    return (off_t)block * EXT2_BLOCK_SIZE;
}

/**
 * Lê um bloco completo do sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser lido
 * @param buf Buffer para armazenar os dados lidos (deve ter pelo menos EXT2_BLOCK_SIZE bytes)
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_block(ext2_fs_t *fs, uint32_t block, void *buf)
{
    ssize_t bytes_read = pread(fs->fd, buf, EXT2_BLOCK_SIZE, fs_block_offset(fs, block));
    if (bytes_read == EXT2_BLOCK_SIZE)
        return 0;
    return -1;
}

/**
 * Escreve um bloco completo no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser escrito
 * @param buf Buffer contendo os dados a serem escritos (deve ter EXT2_BLOCK_SIZE bytes)
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_block(ext2_fs_t *fs, uint32_t block, void *buf)
{
    ssize_t bytes_written = pwrite(fs->fd, buf, EXT2_BLOCK_SIZE, fs_block_offset(fs, block));
    if (bytes_written == EXT2_BLOCK_SIZE)
        return 0;
    return -1;
}

/*
 * ============================================================================
 * GROUP DESCRIPTOR MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Calcula o offset do descritor de grupo especificado.
 *
 * @param group Número do grupo (baseado em 0)
 * @return Offset em bytes do descritor de grupo no dispositivo
 *
 * Os descritores de grupo ficam logo após o superbloco na tabela de descritores.
 */
off_t gd_offset(uint32_t group)
{
    // Tabela de descritores inicia logo após o bloco do superbloco
    off_t group_desc_table_offset = EXT2_SUPER_OFFSET + EXT2_BLOCK_SIZE;
    return group_desc_table_offset + group * sizeof(struct ext2_group_desc);
}

/**
 * Lê o descritor de um grupo específico.
 *
 * @param fs Estrutura do filesystem
 * @param group Número do grupo (baseado em 0)
 * @param gd Buffer para armazenar o descritor lido
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_group_desc(ext2_fs_t *fs, uint32_t group, struct ext2_group_desc *gd)
{
    ssize_t bytes_read = pread(fs->fd, gd, sizeof(*gd), gd_offset(group));
    if (bytes_read == (ssize_t)sizeof(*gd))
        return 0;
    return -1;
}

/**
 * Escreve o descritor de um grupo específico.
 *
 * @param fs Estrutura do filesystem
 * @param group Número do grupo (baseado em 0)
 * @param gd Descritor de grupo a ser escrito
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_group_desc(ext2_fs_t *fs, uint32_t group, struct ext2_group_desc *gd)
{
    ssize_t bytes_written = pwrite(fs->fd, gd, sizeof(*gd), gd_offset(group));
    if (bytes_written == (ssize_t)sizeof(*gd))
        return 0;
    return -1;
}

/*
 * ============================================================================
 * INODE MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Localiza a posição de um inode no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param ino Número do inode (baseado em 1)
 * @param gd_out Buffer para retornar o descritor de grupo correspondente
 * @param off Buffer para retornar o offset do inode no dispositivo
 * @return 0 em sucesso, -1 em erro
 *
 * Esta função determina em qual grupo o inode está localizado e
 * calcula seu offset exato na tabela de inodes desse grupo.
 */
int inode_loc(ext2_fs_t *fs, uint32_t ino, struct ext2_group_desc *gd_out, off_t *off)
{
    // Inode 0 é inválido no EXT2 (inodes começam em 1)
    if (ino == 0)
    {
        errno = EINVAL;
        return -1;
    }

    // Converte número do inode para índice baseado em 0
    uint32_t inode_index = ino - 1;

    // Determina o grupo e o índice dentro do grupo
    uint32_t group = inode_index / fs->sb.s_inodes_per_group;
    uint32_t index_in_group = inode_index % fs->sb.s_inodes_per_group;

    // Lê o descritor de grupo correspondente
    if (fs_read_group_desc(fs, group, gd_out) < 0)
        return -1;

    // Calcula o offset exato do inode na tabela de inodes
    off_t inode_table_offset = fs_block_offset(fs, gd_out->bg_inode_table);
    *off = inode_table_offset + (off_t)index_in_group * fs->sb.s_inode_size;

    return 0;
}

/**
 * Lê um inode específico do sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser lido
 * @param inode Buffer para armazenar o inode lido
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_inode(ext2_fs_t *fs, uint32_t ino, struct ext2_inode *inode)
{
    struct ext2_group_desc group_desc;
    off_t inode_offset;

    // Localiza o descritor de grupo e o offset do inode
    if (inode_loc(fs, ino, &group_desc, &inode_offset) < 0)
        return -1;

    // Lê o inode do disco para a estrutura fornecida
    ssize_t bytes_read = pread(fs->fd, inode, sizeof(*inode), inode_offset);
    if (bytes_read != (ssize_t)sizeof(*inode))
        return -1;

    return 0;
}

/**
 * Escreve um inode específico no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser escrito
 * @param inode Estrutura do inode a ser escrita
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_inode(ext2_fs_t *fs, uint32_t ino, struct ext2_inode *inode)
{
    struct ext2_group_desc group_desc;
    off_t inode_offset;

    // Localiza o descritor de grupo e o offset do inode
    if (inode_loc(fs, ino, &group_desc, &inode_offset) < 0)
        return -1;

    // Escreve o inode no disco a partir da estrutura fornecida
    ssize_t bytes_written = pwrite(fs->fd, inode, sizeof(*inode), inode_offset);
    if (bytes_written != (ssize_t)sizeof(*inode))
        return -1;
    return 0;
}

/**
 * Aloca um novo inode no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param mode Modo do arquivo (usado para determinar se é diretório)
 * @param out_ino Buffer para retornar o número do inode alocado
 * @return 0 em sucesso, -1 em erro (ENOSPC se não há inodes livres)
 *
 * Esta função procura por um inode livre em todos os grupos,
 * atualiza os bitmaps e contadores apropriados.
 */
int fs_alloc_inode(ext2_fs_t *fs, uint16_t mode, uint32_t *out_ino)
{
    uint8_t bitmap[EXT2_BLOCK_SIZE];

    // Percorre todos os grupos de blocos procurando por inodes livres
    for (uint32_t group = 0; group < fs->groups_count; ++group)
    {
        struct ext2_group_desc gd;
        if (fs_read_group_desc(fs, group, &gd) < 0)
            return -1;

        // Se não há inodes livres neste grupo, pula para o próximo
        if (gd.bg_free_inodes_count == 0)
            continue;

        // Lê o bitmap de inodes do grupo
        if (fs_read_block(fs, gd.bg_inode_bitmap, bitmap) < 0)
            return -1;

        // Procura por um inode livre no bitmap (bit = 0 indica livre)
        for (uint32_t idx = 0; idx < fs->sb.s_inodes_per_group; ++idx)
        {
            if (!(bitmap[BIT_BYTE(idx)] & BIT_MASK(idx)))
            {
                // Marca o inode como usado no bitmap (set bit = 1)
                bitmap[BIT_BYTE(idx)] |= BIT_MASK(idx);
                if (fs_write_block(fs, gd.bg_inode_bitmap, bitmap) < 0)
                    return -1;

                // Atualiza contadores no descritor de grupo
                gd.bg_free_inodes_count--;
                if ((mode & EXT2_S_IFDIR) == EXT2_S_IFDIR)
                    gd.bg_used_dirs_count++; // Incrementa contador de diretórios se necessário

                if (fs_write_group_desc(fs, group, &gd) < 0)
                    return -1;

                // Atualiza contador global no superbloco
                fs->sb.s_free_inodes_count--;
                fs_sync_super(fs);

                // Calcula o número absoluto do inode (inodes começam em 1)
                *out_ino = group * fs->sb.s_inodes_per_group + idx + 1;
                return 0;
            }
        }
    }

    // Nenhum inode livre encontrado
    errno = ENOSPC;
    return -1;
}

/**
 * Libera um inode no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser liberado
 * @return 0 em sucesso, -1 em erro
 *
 * Esta função marca o inode como livre no bitmap e atualiza
 * os contadores nos descritores de grupo e superbloco.
 */
int fs_free_inode(ext2_fs_t *fs, uint32_t ino)
{
    struct ext2_group_desc gd;
    off_t inode_offset;

    // Localiza o grupo e o offset do inode
    if (inode_loc(fs, ino, &gd, &inode_offset) < 0)
        return -1;

    // Lê o bitmap de inodes do grupo
    uint8_t bitmap[EXT2_BLOCK_SIZE];
    if (fs_read_block(fs, gd.bg_inode_bitmap, bitmap) < 0)
        return -1;

    // Calcula o índice do inode dentro do grupo
    uint32_t index_in_group = (ino - 1) % fs->sb.s_inodes_per_group;

    // Verifica se o inode já está livre (erro se tentar liberar inode já livre)
    if (!(bitmap[BIT_BYTE(index_in_group)] & BIT_MASK(index_in_group)))
    {
        errno = EINVAL;
        return -1;
    }

    // Marca o inode como livre no bitmap (clear bit = 0)
    bitmap[BIT_BYTE(index_in_group)] &= ~BIT_MASK(index_in_group);
    if (fs_write_block(fs, gd.bg_inode_bitmap, bitmap) < 0)
        return -1;

    // Atualiza contadores de inodes livres
    gd.bg_free_inodes_count++;
    fs->sb.s_free_inodes_count++;

    // Persiste alterações no descritor de grupo e superbloco
    uint32_t group = (ino - 1) / fs->sb.s_inodes_per_group;
    if (fs_write_group_desc(fs, group, &gd) < 0)
        return -1;

    return fs_sync_super(fs);
}

/*
 * ============================================================================
 * BLOCK ALLOCATION FUNCTIONS
 * ============================================================================
 */

/**
 * Aloca um novo bloco no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param out_block Buffer para retornar o número do bloco alocado
 * @return 0 em sucesso, -1 em erro (ENOSPC se não há blocos livres)
 *
 * Procura por um bloco livre em todos os grupos, atualiza bitmaps e contadores.
 */
int fs_alloc_block(ext2_fs_t *fs, uint32_t *out_block)
{
    uint8_t bitmap[EXT2_BLOCK_SIZE];

    // Percorre todos os grupos de blocos procurando por blocos livres
    for (uint32_t group = 0; group < fs->groups_count; ++group)
    {
        struct ext2_group_desc gd;
        if (fs_read_group_desc(fs, group, &gd) < 0)
            return -1;

        // Se não há blocos livres neste grupo, pula para o próximo
        if (gd.bg_free_blocks_count == 0)
            continue;

        // Lê o bitmap de blocos do grupo
        if (fs_read_block(fs, gd.bg_block_bitmap, bitmap) < 0)
            return -1;

        // Procura por um bloco livre no bitmap (bit = 0 indica livre)
        for (uint32_t idx = 0; idx < fs->sb.s_blocks_per_group; ++idx)
        {
            if (!(bitmap[BIT_BYTE(idx)] & BIT_MASK(idx)))
            {
                // Marca o bloco como usado no bitmap (set bit = 1)
                bitmap[BIT_BYTE(idx)] |= BIT_MASK(idx);
                if (fs_write_block(fs, gd.bg_block_bitmap, bitmap) < 0)
                    return -1;

                // Atualiza contadores
                gd.bg_free_blocks_count--;
                if (fs_write_group_desc(fs, group, &gd) < 0)
                    return -1;

                fs->sb.s_free_blocks_count--;
                fs_sync_super(fs);

                // Calcula o número absoluto do bloco
                *out_block = group * fs->sb.s_blocks_per_group + idx;
                return 0;
            }
        }
    }

    // Nenhum bloco livre encontrado
    errno = ENOSPC;
    return -1;
}

/**
 * Libera um bloco no sistema de arquivos.
 *
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser liberado
 * @return 0 em sucesso, -1 em erro
 */
int fs_free_block(ext2_fs_t *fs, uint32_t block)
{
    // Determina o grupo e o índice do bloco dentro do grupo
    uint32_t group = block / fs->sb.s_blocks_per_group;
    uint32_t index_in_group = block % fs->sb.s_blocks_per_group;

    // Lê o descritor de grupo correspondente
    struct ext2_group_desc gd;
    if (fs_read_group_desc(fs, group, &gd) < 0)
        return -1;

    // Lê o bitmap de blocos do grupo
    uint8_t bitmap[EXT2_BLOCK_SIZE];
    if (fs_read_block(fs, gd.bg_block_bitmap, bitmap) < 0)
        return -1;

    // Verifica se o bloco já está livre (erro se tentar liberar bloco já livre)
    if (!(bitmap[BIT_BYTE(index_in_group)] & BIT_MASK(index_in_group)))
    {
        errno = EINVAL;
        return -1;
    }

    // Marca o bloco como livre no bitmap (clear bit = 0)
    bitmap[BIT_BYTE(index_in_group)] &= ~BIT_MASK(index_in_group);
    if (fs_write_block(fs, gd.bg_block_bitmap, bitmap) < 0)
        return -1;

    // Atualiza contadores de blocos livres
    gd.bg_free_blocks_count++;
    fs->sb.s_free_blocks_count++;

    // Persiste alterações
    if (fs_write_group_desc(fs, group, &gd) < 0)
        return -1;
    return fs_sync_super(fs);
}

/**
 * Libera todos os blocos de dados associados a um inode.
 *
 * @param fs Estrutura do filesystem
 * @param inode Estrutura do inode cujos blocos serão liberados
 * @return 0 em sucesso, valor negativo em erro
 *
 * Esta função libera blocos diretos e blocos indiretos simples.
 * Não implementa liberação de blocos duplamente ou triplamente indiretos.
 */
int free_inode_blocks(ext2_fs_t *fs, struct ext2_inode *inode)
{
    // Libera os 12 blocos diretos
    for (int i = 0; i < 12; ++i)
    {
        uint32_t bloco = inode->i_block[i];
        if (bloco)
            fs_free_block(fs, bloco);
    }

    // Libera blocos indiretos simples (bloco 12)
    uint32_t bloco_indireto = inode->i_block[12];
    if (bloco_indireto)
    {
        // Lê o bloco indireto que contém ponteiros para blocos de dados
        uint32_t blocos[EXT2_BLOCK_SIZE / sizeof(uint32_t)];
        if (fs_read_block(fs, bloco_indireto, blocos) == 0)
        {
            // Libera todos os blocos referenciados pelo bloco indireto
            for (size_t j = 0; j < EXT2_BLOCK_SIZE / sizeof(uint32_t); ++j)
            {
                if (blocos[j])
                    fs_free_block(fs, blocos[j]);
            }
        }
        // Libera o próprio bloco indireto
        fs_free_block(fs, bloco_indireto);
    }

    // TODO: Implementar liberação de blocos duplamente e triplamente indiretos
    return 0;
}

/*
 * ============================================================================
 * DIRECTORY MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Itera sobre as entradas de um diretório, chamando callback para cada entrada.
 *
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório a ser iterado
 * @param cb Função callback a ser chamada para cada entrada
 * @param user Dados do usuário passados para o callback
 * @return 0 em sucesso, valor positivo se callback pediu parada, -1 em erro
 *
 * Esta função percorre apenas os blocos diretos do diretório.
 */
int fs_iterate_dir(ext2_fs_t *fs, struct ext2_inode *dir_inode, dir_iter_cb cb, void *user)
{
    // Verifica se o inode é realmente um diretório
    if (!ext2_is_dir(dir_inode))
    {
        errno = ENOTDIR;
        return -1;
    }

    uint8_t block_buf[EXT2_BLOCK_SIZE];

    // Percorre apenas os 12 blocos diretos do diretório
    for (int i = 0; i < 12; ++i)
    {
        uint32_t block_num = dir_inode->i_block[i];
        if (!block_num)
            continue; // Pula blocos não alocados

        if (fs_read_block(fs, block_num, block_buf) < 0)
            return -1;

        // Processa entradas de diretório dentro do bloco
        uint32_t offset = 0;
        while (offset < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(block_buf + offset);

            // Entrada com rec_len = 0 indica corrupção, para a leitura
            if (entry->rec_len == 0)
                break;

            // Chama callback apenas para entradas válidas (inode != 0)
            if (entry->inode && cb)
            {
                int stop = cb(entry, user);
                if (stop)
                    return stop; // Callback pediu para parar a iteração
            }
            offset += entry->rec_len;
        }
    }
    return 0;
}

/**
 * Estrutura de contexto para busca de arquivos em diretório.
 */
struct dir_find_ctx
{
    char *name;   // Nome a ser buscado
    uint32_t ino; // Inode encontrado (0 se não encontrado)
};

/**
 * Callback para busca de arquivo específico em diretório.
 *
 * @param entry Entrada de diretório atual
 * @param user_ctx Contexto de busca (struct dir_find_ctx)
 * @return 1 se encontrou o arquivo, 0 para continuar procurando
 */
static int find_cb(struct ext2_dir_entry *entry, void *user_ctx)
{
    struct dir_find_ctx *ctx = user_ctx;
    size_t name_len = strlen(ctx->name);

    // Compara nome da entrada com nome procurado
    if (entry->name_len == name_len && strncmp(entry->name, ctx->name, name_len) == 0)
    {
        ctx->ino = entry->inode;
        return 1; // Encontrou, para a busca
    }
    return 0; // Não encontrou, continua
}

/**
 * Procura por um arquivo específico dentro de um diretório.
 *
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório onde procurar
 * @param name Nome do arquivo a ser procurado
 * @param out_ino Buffer para retornar o número do inode encontrado
 * @return 0 em sucesso, -1 em erro (ENOENT se não encontrado)
 */
int fs_find_in_dir(ext2_fs_t *fs, struct ext2_inode *dir_inode, char *name, uint32_t *out_ino)
{
    struct dir_find_ctx ctx = {.name = name, .ino = 0};

    // Itera sobre o diretório usando o callback de busca
    if (fs_iterate_dir(fs, dir_inode, find_cb, &ctx) < 0)
        return -1;

    if (ctx.ino == 0)
    {
        errno = ENOENT; // Arquivo não encontrado
        return -1;
    }

    *out_ino = ctx.ino;
    return 0;
}

/**
 * Calcula o tamanho necessário para uma entrada de diretório.
 *
 * @param name_len Comprimento do nome do arquivo
 * @return Tamanho da entrada alinhado a 4 bytes
 *
 * Fórmula: (8 bytes fixos + nome + padding) alinhado a 4 bytes
 */
uint16_t rec_len_needed(uint8_t name_len)
{
    return (uint16_t)((8 + name_len + 3) & ~3);
}

/*
 * ============================================================================
 * PATH RESOLUTION FUNCTIONS
 * ============================================================================
 */

/**
 * Resolve um caminho absoluto para um número de inode.
 *
 * @param fs Estrutura do filesystem
 * @param path Caminho absoluto a ser resolvido (deve começar com '/')
 * @param ino Buffer para retornar o número do inode encontrado
 * @return 0 em sucesso, -1 em erro
 *
 * Esta função quebra o caminho em componentes e percorre
 * a árvore de diretórios para encontrar o arquivo final.
 */
int fs_path_resolve(ext2_fs_t *fs, char *path, uint32_t *ino)
{
    if (!path || !*path)
    {
        errno = EINVAL;
        return -1;
    }

    uint32_t current_ino = EXT2_ROOT_INO; // Começa na raiz

    // Caso especial: caminho é apenas a raiz "/"
    if (strcmp(path, "/") == 0)
    {
        *ino = current_ino;
        return 0;
    }

    // Cria uma cópia do caminho para tokenização (strtok_r modifica a string)
    char *path_copy = strdup(path);
    if (!path_copy)
        return -1;

    char *saveptr = NULL;
    char *token = strtok_r(path_copy, "/", &saveptr); // Quebra por '/'

    // Percorre cada componente do caminho
    while (token)
    {
        struct ext2_inode dir_inode;
        if (fs_read_inode(fs, current_ino, &dir_inode) < 0)
        {
            free(path_copy);
            return -1;
        }

        // Verifica se o inode atual é um diretório
        if (!ext2_is_dir(&dir_inode))
        {
            free(path_copy);
            errno = ENOTDIR;
            return -1;
        }

        // Procura o próximo componente no diretório atual
        if (fs_find_in_dir(fs, &dir_inode, token, &current_ino) < 0)
        {
            free(path_copy);
            return -1;
        }

        token = strtok_r(NULL, "/", &saveptr); // Próximo componente
    }

    free(path_copy);
    *ino = current_ino;
    return 0;
}

/**
 * Estrutura de contexto para busca reversa de nome de arquivo.
 */
struct child_ctx
{
    uint32_t ino;   // Inode do filho a ser buscado
    char name[256]; // Nome do filho encontrado
};

/**
 * Callback para busca reversa do nome de um inode específico.
 *
 * @param entry Entrada de diretório atual
 * @param user_ctx Contexto de busca (struct child_ctx)
 * @return 1 se encontrou o inode, 0 para continuar procurando
 */
static int child_cb(struct ext2_dir_entry *entry, void *user_ctx)
{
    struct child_ctx *ctx = user_ctx;

    if (entry->inode == ctx->ino)
    {
        // Copia o nome encontrado, limitando o tamanho
        size_t len = entry->name_len;
        if (len >= sizeof(ctx->name))
            len = sizeof(ctx->name) - 1;

        memcpy(ctx->name, entry->name, len);
        ctx->name[len] = '\0'; // Garante terminação nula
        return 1;              // Encontrou, para a busca
    }
    return 0;
}

/**
 * Constrói o caminho absoluto de um inode específico.
 *
 * @param fs Estrutura do filesystem
 * @param ino Número do inode
 * @return String com o caminho absoluto (deve ser liberada com free), ou NULL em erro
 *
 * Esta função percorre a árvore de diretórios de baixo para cima,
 * construindo o caminho completo do arquivo.
 */
char *fs_get_path(ext2_fs_t *fs, uint32_t ino)
{
    // Caso especial: inode da raiz
    if (ino == EXT2_ROOT_INO)
        return strdup("/");

    char *components[64]; // Limita profundidade máxima
    int count = 0;
    uint32_t current_ino = ino;

    // Percorre para cima na árvore de diretórios até a raiz
    while (current_ino != EXT2_ROOT_INO && count < 64)
    {
        struct ext2_inode inode;
        if (fs_read_inode(fs, current_ino, &inode) < 0)
            return NULL;

        // Encontra o inode do diretório pai usando ".."
        uint32_t parent_ino;
        if (fs_find_in_dir(fs, &inode, "..", &parent_ino) < 0)
            return NULL;

        // Lê o inode do pai
        struct ext2_inode parent_inode;
        if (fs_read_inode(fs, parent_ino, &parent_inode) < 0)
            return NULL;

        // Procura o nome do arquivo atual no diretório pai
        struct child_ctx ctx = {.ino = current_ino};
        if (fs_iterate_dir(fs, &parent_inode, child_cb, &ctx) < 0)
            return NULL;

        // Armazena o componente do caminho
        components[count++] = strdup(ctx.name);
        current_ino = parent_ino; // Sobe um nível
    }

    // Calcula o tamanho total necessário para o caminho
    size_t total_len = 1; // Para '/' inicial
    for (int i = count - 1; i >= 0; --i)
        total_len += strlen(components[i]) + 1; // +1 para '/'

    // Aloca e constrói o caminho final
    char *path = malloc(total_len + 1);
    if (!path)
        return NULL;

    char *ptr = path;
    *ptr++ = '/'; // Inicia com '/'

    // Adiciona componentes na ordem correta (do topo para baixo)
    for (int i = count - 1; i >= 0; --i)
    {
        size_t len = strlen(components[i]);
        memcpy(ptr, components[i], len);
        ptr += len;
        if (i) // Não adiciona '/' após o último componente
            *ptr++ = '/';
        free(components[i]); // Libera componente temporário
    }
    *ptr = '\0'; // Termina string

    return path;
}

/**
 * Junta um caminho base com um caminho relativo.
 *
 * @param fs Estrutura do filesystem
 * @param cwd Inode do diretório atual (base)
 * @param rel Caminho relativo a ser adicionado
 * @return String com o caminho absoluto resultante (deve ser liberada), ou NULL em erro
 *
 * Se rel já for absoluto (começar com '/'), apenas duplica rel.
 * Caso contrário, obtém o caminho de cwd e adiciona rel.
 */
char *fs_join_path(ext2_fs_t *fs, uint32_t cwd, const char *rel)
{
    if (!rel || !*rel)
        return NULL;

    // Se o caminho relativo já é absoluto, apenas duplica
    if (rel[0] == '/')
        return strdup(rel);

    // Obtém o caminho absoluto do diretório atual
    char *base = fs_get_path(fs, cwd);
    if (!base)
        return NULL;

    size_t base_len = strlen(base);
    size_t rel_len = strlen(rel);

    // Verifica se precisa adicionar '/' entre base e rel
    int add_slash = (base_len > 1 && base[base_len - 1] != '/');

    // Aloca espaço para o novo caminho
    size_t total_len = base_len + add_slash + rel_len + 1;
    char *full = malloc(total_len);
    if (!full)
    {
        free(base);
        return NULL;
    }

    // Constrói o caminho completo
    strcpy(full, base);
    if (add_slash)
        strcat(full, "/");
    strcat(full, rel);

    free(base);
    return full;
}

/*
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * Verifica se um nome já existe em um diretório.
 *
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório onde verificar
 * @param name Nome a ser verificado
 * @return 1 se existe, 0 se não existe, -1 em erro
 *
 * Esta função é útil para evitar duplicatas ao criar novos arquivos.
 */
int name_exists(ext2_fs_t *fs, struct ext2_inode *dir_inode, char *name)
{
    if (!ext2_is_dir(dir_inode))
    {
        errno = ENOTDIR;
        return -1;
    }

    size_t name_len = strlen(name);

    // Percorre apenas os blocos diretos do diretório
    for (int i = 0; i < 12; ++i)
    {
        uint32_t block_num = dir_inode->i_block[i]; // Obtém o número do bloco
        if (!block_num)
            continue;

        uint8_t block_buf[EXT2_BLOCK_SIZE];
        if (fs_read_block(fs, block_num, block_buf) < 0) // Lê o bloco do diretório
            return -1;

        uint32_t offset = 0;
        while (offset < EXT2_BLOCK_SIZE)
        {
            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(block_buf + offset); // Obtém a entrada de diretório
            if (entry->rec_len == 0)
                break;

            // Verifica se encontrou uma entrada com o mesmo nome
            if (entry->inode && entry->name_len == name_len &&
                strncmp(entry->name, name, name_len) == 0)
                return 1; // Nome existe

            offset += entry->rec_len; // Avança para a próxima entrada
        }
    }
    return 0; // Nome não existe
}

/**
 * Sincroniza o superbloco para disco.
 *
 * @param fs Estrutura do filesystem
 * @return 0 em sucesso, -1 em erro
 *
 * Esta função garante que alterações no superbloco sejam
 * persistidas no dispositivo de armazenamento.
 */
int fs_sync_super(ext2_fs_t *fs)
{
    ssize_t bytes_written = pwrite(fs->fd, &fs->sb, sizeof(fs->sb), EXT2_SUPER_OFFSET);
    if (bytes_written == (ssize_t)sizeof(fs->sb))
        return 0;
    return -1;
}

/**
 * Imprime informações de uma entrada de diretório (função de debug).
 *
 * @param e Entrada de diretório a ser impressa
 *
 * Esta função formata e exibe informações detalhadas sobre uma entrada,
 * incluindo colorização baseada no tipo de arquivo.
 */
void print_entry(struct ext2_dir_entry *e)
{
    char name[256];
    size_t n = e->name_len < 255 ? e->name_len : 255;
    memcpy(name, e->name, n);
    name[n] = '\0';

    // Colorização baseada no tipo de arquivo
    if (e->file_type == 1)                   // Arquivo regular
        printf("\033[32m%s\033[0m\n", name); // Verde
    else if (e->file_type == 2)              // Diretório
        printf("\033[34m%s\033[0m\n", name); // Azul

    printf("inode: %u\n", e->inode);
    printf("record length: %u\n", e->rec_len);
    printf("name length: %u\n", e->name_len);
    printf("file type: %u\n", e->file_type);
    printf("\n");
}
