/**
 * ============================================================================
 * EXT2 FILESYSTEM UTILITIES HEADER
 * ============================================================================
 *
 * Este arquivo define todas as estruturas, constantes e funções necessárias
 * para manipulação de sistemas de arquivos EXT2. Inclui definições de baixo
 * nível para acesso direto ao formato de dados EXT2 em imagens de disco.
 *
 * O EXT2 (Second Extended Filesystem) é um sistema de arquivos clássico do
 * Linux que organiza dados em blocos, inodes e grupos de blocos.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

/*
 * ============================================================================
 * EXT2 FILESYSTEM CONSTANTS
 * ============================================================================
 */

/** Assinatura mágica do superbloco EXT2 para validação */
#define EXT2_SUPER_MAGIC 0xEF53

/** Tamanho fixo de bloco no EXT2 (1024 bytes) */
#define EXT2_BLOCK_SIZE 1024

/** Número de ponteiros por bloco indireto (1024 / 4 bytes) */
#define PTRS_PER_BLOCK 256

/** Total de ponteiros de bloco em um inode (12 diretos + 3 indiretos) */
#define EXT2_N_BLOCKS 15

/** Comprimento máximo de nome de arquivo */
#define EXT2_NAME_LEN 255

/** Offset do superbloco na imagem do filesystem (1024 bytes do início) */
#define EXT2_SUPER_OFFSET 1024

/*
 * ============================================================================
 * FILE MODE CONSTANTS (i_mode field values)
 * ============================================================================
 */

/* Tipos de arquivo (formato) */
#define EXT2_S_IFSOCK 0xC000 /**< Socket */
#define EXT2_S_IFLNK 0xA000  /**< Link simbólico */
#define EXT2_S_IFREG 0x8000  /**< Arquivo regular */
#define EXT2_S_IFBLK 0x6000  /**< Dispositivo de bloco */
#define EXT2_S_IFDIR 0x4000  /**< Diretório */
#define EXT2_S_IFCHR 0x2000  /**< Dispositivo de caractere */
#define EXT2_S_IFIFO 0x1000  /**< FIFO (named pipe) */

/* Permissões de acesso (proprietário, grupo, outros) */
#define EXT2_S_IRUSR 0x0100 /**< Leitura pelo proprietário */
#define EXT2_S_IWUSR 0x0080 /**< Escrita pelo proprietário */
#define EXT2_S_IXUSR 0x0040 /**< Execução pelo proprietário */
#define EXT2_S_IRGRP 0x0020 /**< Leitura pelo grupo */
#define EXT2_S_IWGRP 0x0010 /**< Escrita pelo grupo */
#define EXT2_S_IXGRP 0x0008 /**< Execução pelo grupo */
#define EXT2_S_IROTH 0x0004 /**< Leitura por outros */
#define EXT2_S_IWOTH 0x0002 /**< Escrita por outros */
#define EXT2_S_IXOTH 0x0001 /**< Execução por outros */

/*
 * ============================================================================
 * DIRECTORY ENTRY FILE TYPES
 * ============================================================================
 */

#define EXT2_FT_UNKNOWN 0  /**< Tipo desconhecido */
#define EXT2_FT_REG_FILE 1 /**< Arquivo regular */
#define EXT2_FT_DIR 2      /**< Diretório */
#define EXT2_FT_CHRDEV 3   /**< Dispositivo de caractere */
#define EXT2_FT_BLKDEV 4   /**< Dispositivo de bloco */
#define EXT2_FT_FIFO 5     /**< FIFO */
#define EXT2_FT_SOCK 6     /**< Socket */
#define EXT2_FT_SYMLINK 7  /**< Link simbólico */

/*
 * ============================================================================
 * RESERVED INODE NUMBERS
 * ============================================================================
 */

#define EXT2_BAD_INO 1  /**< Inode para blocos defeituosos */
#define EXT2_ROOT_INO 2 /**< Inode do diretório raiz */

/*
 * ============================================================================
 * BITMAP MANIPULATION MACROS
 * ============================================================================
 */

/** Calcula o byte que contém o bit especificado */
#define BIT_BYTE(b) ((b) >> 3)

/** Calcula a máscara para o bit dentro do byte */
#define BIT_MASK(b) (1U << ((b) & 7))

/*
 * ============================================================================
 * EXT2 DATA STRUCTURES
 * ============================================================================
 */

/**
 * Estrutura do superbloco EXT2.
 *
 * O superbloco contém metadados globais sobre o sistema de arquivos,
 * incluindo contadores de recursos, parâmetros de configuração e
 * informações sobre a organização dos dados.
 */
struct ext2_super_block
{
    uint32_t s_inodes_count;          /**< Total de inodes no filesystem */
    uint32_t s_blocks_count;          /**< Total de blocos no filesystem */
    uint32_t s_r_blocks_count;        /**< Blocos reservados para superusuário */
    uint32_t s_free_blocks_count;     /**< Número de blocos livres */
    uint32_t s_free_inodes_count;     /**< Número de inodes livres */
    uint32_t s_first_data_block;      /**< Primeiro bloco de dados */
    uint32_t s_log_block_size;        /**< Log2(tamanho_bloco) - 10 */
    uint32_t s_log_frag_size;         /**< Log2(tamanho_fragmento) - 10 */
    uint32_t s_blocks_per_group;      /**< Blocos por grupo */
    uint32_t s_frags_per_group;       /**< Fragmentos por grupo */
    uint32_t s_inodes_per_group;      /**< Inodes por grupo */
    uint32_t s_mtime;                 /**< Timestamp da última montagem */
    uint32_t s_wtime;                 /**< Timestamp da última escrita */
    uint16_t s_mnt_count;             /**< Contador de montagens desde fsck */
    uint16_t s_max_mnt_count;         /**< Máximo de montagens antes de fsck */
    uint16_t s_magic;                 /**< Assinatura mágica (0xEF53) */
    uint16_t s_state;                 /**< Estado do filesystem */
    uint16_t s_errors;                /**< Comportamento em caso de erro */
    uint16_t s_minor_rev_level;       /**< Nível de revisão menor */
    uint32_t s_lastcheck;             /**< Timestamp da última verificação */
    uint32_t s_checkinterval;         /**< Intervalo máximo entre verificações */
    uint32_t s_creator_os;            /**< SO que criou o filesystem */
    uint32_t s_rev_level;             /**< Nível de revisão */
    uint16_t s_def_resuid;            /**< UID padrão para blocos reservados */
    uint16_t s_def_resgid;            /**< GID padrão para blocos reservados */
    uint32_t s_first_ino;             /**< Primeiro inode não reservado */
    uint16_t s_inode_size;            /**< Tamanho da estrutura de inode */
    uint16_t s_block_group_nr;        /**< Número do grupo deste superbloco */
    uint32_t s_feature_compat;        /**< Características compatíveis */
    uint32_t s_feature_incompat;      /**< Características incompatíveis */
    uint32_t s_feature_ro_compat;     /**< Características somente leitura */
    uint8_t s_uuid[16];               /**< UUID do volume */
    char s_volume_name[16];           /**< Nome do volume */
    char s_last_mounted[64];          /**< Último ponto de montagem */
    uint32_t s_algo_bitmap;           /**< Algoritmo de compressão usado */
    uint8_t s_prealloc_blocks;        /**< Blocos a pré-alocar para arquivos */
    uint8_t s_prealloc_dir_blocks;    /**< Blocos a pré-alocar para diretórios */
    uint16_t s_alignment;             /**< Alinhamento */
    uint8_t s_journal_uuid[16];       /**< UUID do journal */
    uint32_t s_journal_inum;          /**< Número do inode do journal */
    uint32_t s_journal_dev;           /**< Dispositivo do journal */
    uint32_t s_last_orphan;           /**< Início da lista de inodes órfãos */
    uint32_t s_hash_seed[4];          /**< Seeds para hash de diretórios */
    uint8_t s_def_hash_version;       /**< Versão padrão do hash */
    uint8_t s_reserved_char_pad;      /**< Padding de caractere */
    uint16_t s_reserved_word_pad;     /**< Padding de word */
    uint32_t s_default_mount_options; /**< Opções padrão de montagem */
    uint32_t s_first_meta_bg;         /**< Primeiro metabloco de grupo */
    uint8_t s_reserved[760];          /**< Espaço reservado para expansão */
} __attribute__((packed));

/**
 * Descritor de grupo de blocos.
 *
 * Cada grupo de blocos tem um descritor que especifica a localização
 * de suas estruturas de dados (bitmaps, tabela de inodes) e contadores
 * de recursos livres.
 */
struct ext2_group_desc
{
    uint32_t bg_block_bitmap;      /**< Número do bloco do bitmap de blocos */
    uint32_t bg_inode_bitmap;      /**< Número do bloco do bitmap de inodes */
    uint32_t bg_inode_table;       /**< Número do primeiro bloco da tabela de inodes */
    uint16_t bg_free_blocks_count; /**< Número de blocos livres no grupo */
    uint16_t bg_free_inodes_count; /**< Número de inodes livres no grupo */
    uint16_t bg_used_dirs_count;   /**< Número de diretórios no grupo */
    uint16_t bg_pad;               /**< Padding para alinhamento */
    uint8_t bg_reserved[12];       /**< Espaço reservado */
} __attribute__((packed));

/**
 * Estrutura de inode EXT2.
 *
 * O inode contém todos os metadados de um arquivo ou diretório,
 * incluindo permissões, timestamps, tamanho e ponteiros para blocos de dados.
 */
struct ext2_inode
{
    uint16_t i_mode;                 /**< Modo do arquivo (tipo + permissões) */
    uint16_t i_uid;                  /**< UID do proprietário */
    uint32_t i_size;                 /**< Tamanho em bytes */
    uint32_t i_atime;                /**< Timestamp de último acesso */
    uint32_t i_ctime;                /**< Timestamp de criação */
    uint32_t i_mtime;                /**< Timestamp de última modificação */
    uint32_t i_dtime;                /**< Timestamp de deleção */
    uint16_t i_gid;                  /**< GID do grupo proprietário */
    uint16_t i_links_count;          /**< Número de hard links */
    uint32_t i_blocks;               /**< Número de blocos alocados */
    uint32_t i_flags;                /**< Flags do arquivo */
    uint32_t i_osd1;                 /**< Específico do SO */
    uint32_t i_block[EXT2_N_BLOCKS]; /**< Ponteiros para blocos de dados */
    uint32_t i_generation;           /**< Geração do arquivo (para NFS) */
    uint32_t i_file_acl;             /**< ACL estendida do arquivo */
    uint32_t i_dir_acl;              /**< ACL estendida do diretório */
    uint32_t i_faddr;                /**< Endereço do fragmento */
    uint8_t i_osd2[12];              /**< Específico do SO */
} __attribute__((packed));

/**
 * Entrada de diretório EXT2.
 *
 * Representa um arquivo ou subdiretório dentro de um diretório.
 * As entradas são armazenadas sequencialmente nos blocos de dados do diretório.
 */
struct ext2_dir_entry
{
    uint32_t inode;           /**< Número do inode do arquivo */
    uint16_t rec_len;         /**< Tamanho total desta entrada */
    uint8_t name_len;         /**< Comprimento do nome */
    uint8_t file_type;        /**< Tipo do arquivo (EXT2_FT_*) */
    char name[EXT2_NAME_LEN]; /**< Nome do arquivo (não null-terminated) */
} __attribute__((packed));

/**
 * Estrutura de controle do sistema de arquivos EXT2.
 *
 * Mantém o estado de uma imagem EXT2 aberta, incluindo o descritor
 * de arquivo e dados cache do superbloco.
 */
typedef struct
{
    int fd;                     /**< Descritor de arquivo da imagem */
    struct ext2_super_block sb; /**< Superbloco cache */
    uint32_t groups_count;      /**< Número total de grupos de blocos */
} ext2_fs_t;

/*
 * ============================================================================
 * FILESYSTEM ACCESS FUNCTIONS
 * ============================================================================
 */

/**
 * Abre uma imagem de sistema de arquivos EXT2.
 * @param img_path Caminho para o arquivo de imagem
 * @return Estrutura do filesystem ou NULL em erro
 */
ext2_fs_t *fs_open(char *img_path);

/**
 * Fecha um sistema de arquivos e libera recursos.
 * @param fs Estrutura do filesystem a ser fechada
 */
void fs_close(ext2_fs_t *fs);

/**
 * Calcula o offset em bytes de um bloco específico.
 * @param fs Estrutura do filesystem
 * @param block Número do bloco
 * @return Offset em bytes do início do bloco
 */
off_t fs_block_offset(ext2_fs_t *fs, uint32_t block);

/**
 * Lê um bloco completo do sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser lido
 * @param buf Buffer para armazenar os dados (EXT2_BLOCK_SIZE bytes)
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_block(ext2_fs_t *fs, uint32_t block, void *buf);

/**
 * Escreve um bloco completo no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser escrito
 * @param buf Buffer contendo os dados a serem escritos
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_block(ext2_fs_t *fs, uint32_t block, void *buf);

/*
 * ============================================================================
 * GROUP DESCRIPTOR FUNCTIONS
 * ============================================================================
 */

/**
 * Lê o descritor de um grupo de blocos específico.
 * @param fs Estrutura do filesystem
 * @param group Número do grupo (baseado em 0)
 * @param gd Buffer para armazenar o descritor lido
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_group_desc(ext2_fs_t *fs, uint32_t group, struct ext2_group_desc *gd);

/**
 * Escreve o descritor de um grupo de blocos específico.
 * @param fs Estrutura do filesystem
 * @param group Número do grupo (baseado em 0)
 * @param gd Descritor de grupo a ser escrito
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_group_desc(ext2_fs_t *fs, uint32_t group, struct ext2_group_desc *gd);

/*
 * ============================================================================
 * INODE MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Localiza a posição física de um inode no dispositivo.
 * @param fs Estrutura do filesystem
 * @param ino Número do inode (baseado em 1)
 * @param gd_out Buffer para retornar o descritor de grupo
 * @param off Buffer para retornar o offset do inode
 * @return 0 em sucesso, -1 em erro
 */
int inode_loc(ext2_fs_t *fs, uint32_t ino, struct ext2_group_desc *gd_out, off_t *off);

/**
 * Lê um inode específico do sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser lido
 * @param inode Buffer para armazenar o inode lido
 * @return 0 em sucesso, -1 em erro
 */
int fs_read_inode(ext2_fs_t *fs, uint32_t ino, struct ext2_inode *inode);

/**
 * Escreve um inode específico no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser escrito
 * @param inode Estrutura do inode a ser escrita
 * @return 0 em sucesso, -1 em erro
 */
int fs_write_inode(ext2_fs_t *fs, uint32_t ino, struct ext2_inode *inode);

/**
 * Aloca um novo inode no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param mode Modo do arquivo (tipo + permissões)
 * @param out_ino Buffer para retornar o número do inode alocado
 * @return 0 em sucesso, -1 em erro (ENOSPC se não há inodes livres)
 */
int fs_alloc_inode(ext2_fs_t *fs, uint16_t mode, uint32_t *out_ino);

/**
 * Libera um inode no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param ino Número do inode a ser liberado
 * @return 0 em sucesso, -1 em erro
 */
int fs_free_inode(ext2_fs_t *fs, uint32_t ino);

/**
 * Imprime informações detalhadas de uma entrada de diretório.
 * @param e Entrada de diretório a ser impressa
 */
void print_entry(struct ext2_dir_entry *e);

/*
 * ============================================================================
 * BLOCK ALLOCATION FUNCTIONS
 * ============================================================================
 */

/**
 * Aloca um novo bloco de dados no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param out_block Buffer para retornar o número do bloco alocado
 * @return 0 em sucesso, -1 em erro (ENOSPC se não há blocos livres)
 */
int fs_alloc_block(ext2_fs_t *fs, uint32_t *out_block);

/**
 * Libera um bloco de dados no sistema de arquivos.
 * @param fs Estrutura do filesystem
 * @param block Número do bloco a ser liberado
 * @return 0 em sucesso, -1 em erro
 */
int fs_free_block(ext2_fs_t *fs, uint32_t block);

/**
 * Libera todos os blocos de dados associados a um inode.
 * @param fs Estrutura do filesystem
 * @param inode Estrutura do inode cujos blocos serão liberados
 * @return 0 em sucesso, valor negativo em erro
 */
int free_inode_blocks(ext2_fs_t *fs, struct ext2_inode *inode);

/*
 * ============================================================================
 * DIRECTORY MANAGEMENT FUNCTIONS
 * ============================================================================
 */

/**
 * Callback para iteração sobre entradas de diretório.
 * @param entry Entrada de diretório atual
 * @param user Dados do usuário passados para a iteração
 * @return 0 para continuar, valor positivo para parar, negativo em erro
 */
typedef int (*dir_iter_cb)(struct ext2_dir_entry *entry, void *user);

/**
 * Itera sobre todas as entradas de um diretório.
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório a ser iterado
 * @param cb Função callback chamada para cada entrada
 * @param user Dados do usuário passados para o callback
 * @return 0 em sucesso, valor do callback se parou, -1 em erro
 */
int fs_iterate_dir(ext2_fs_t *fs, struct ext2_inode *dir_inode, dir_iter_cb cb, void *user);

/**
 * Procura por um arquivo específico dentro de um diretório.
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório onde procurar
 * @param name Nome do arquivo a ser procurado
 * @param out_ino Buffer para retornar o número do inode encontrado
 * @return 0 em sucesso, -1 em erro (ENOENT se não encontrado)
 */
int fs_find_in_dir(ext2_fs_t *fs, struct ext2_inode *dir_inode, char *name, uint32_t *out_ino);

/**
 * Calcula o tamanho necessário para uma entrada de diretório.
 * @param name_len Comprimento do nome do arquivo
 * @return Tamanho da entrada alinhado a 4 bytes
 */
uint16_t rec_len_needed(uint8_t name_len);

/*
 * ============================================================================
 * PATH RESOLUTION FUNCTIONS
 * ============================================================================
 */

/**
 * Resolve um caminho absoluto para um número de inode.
 * @param fs Estrutura do filesystem
 * @param path Caminho absoluto a ser resolvido
 * @param ino Buffer para retornar o número do inode encontrado
 * @return 0 em sucesso, -1 em erro
 */
int fs_path_resolve(ext2_fs_t *fs, char *path, uint32_t *ino);

/**
 * Constrói o caminho absoluto de um inode específico.
 * @param fs Estrutura do filesystem
 * @param dir_ino Número do inode
 * @return String com o caminho absoluto (deve ser liberada), ou NULL em erro
 */
char *fs_get_path(ext2_fs_t *fs, uint32_t dir_ino);

/**
 * Junta um caminho base com um caminho relativo.
 * @param fs Estrutura do filesystem
 * @param cwd Inode do diretório atual (base)
 * @param rel Caminho relativo a ser adicionado
 * @return String com o caminho absoluto resultante (deve ser liberada), ou NULL em erro
 */
char *fs_join_path(ext2_fs_t *fs, uint32_t cwd, const char *rel);

/*
 * ============================================================================
 * SYNCHRONIZATION FUNCTIONS
 * ============================================================================
 */

/**
 * Sincroniza o superbloco para disco.
 * @param fs Estrutura do filesystem
 * @return 0 em sucesso, -1 em erro
 */
int fs_sync_super(ext2_fs_t *fs);

/*
 * ============================================================================
 * NAME VERIFICATION FUNCTIONS
 * ============================================================================
 */

/**
 * Verifica se um nome específico já existe em um diretório.
 * @param fs Estrutura do filesystem
 * @param dir_inode Inode do diretório onde verificar
 * @param name Nome a ser verificado
 * @return 1 se existe, 0 se não existe, -1 em erro
 */
int name_exists(ext2_fs_t *fs, struct ext2_inode *dir_inode, char *name);

/*
 * ============================================================================
 * CONVENIENCE INLINE FUNCTIONS
 * ============================================================================
 */

/**
 * Verifica se um inode representa um diretório.
 * @param inode Estrutura do inode a ser verificada
 * @return 1 se é diretório, 0 caso contrário
 */
static inline int ext2_is_dir(struct ext2_inode *inode)
{
    return (inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR;
}

/**
 * Verifica se um inode representa um arquivo regular.
 * @param inode Estrutura do inode a ser verificada
 * @return 1 se é arquivo regular, 0 caso contrário
 */
static inline int ext2_is_reg(struct ext2_inode *inode)
{
    return (inode->i_mode & EXT2_S_IFREG) == EXT2_S_IFREG;
}

#endif /* UTILS_H */
