/**
 * ============================================================================
 * EXT2 FILESYSTEM OPERATIONS HEADER
 * ============================================================================
 *
 * Este arquivo define a interface para comandos de operações do sistema de
 * arquivos EXT2. Inclui definições de tipos, estruturas e declarações de
 * funções para manipulação de arquivos e diretórios.
 *
 * O sistema implementa uma interface similar ao shell Unix, permitindo
 * operações como navegação, criação, remoção e manipulação de arquivos.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "../utils/utils.h"

/*
 * ============================================================================
 * COMMAND SYSTEM TYPES AND STRUCTURES
 * ============================================================================
 */

/**
 * Assinatura de função para comandos do sistema EXT2.
 *
 * Todos os comandos seguem esta interface padronizada:
 *
 * @param argc Número de argumentos da linha de comando (incluindo o nome do comando)
 * @param argv Array de strings com os argumentos da linha de comando
 * @param fs Ponteiro para a estrutura do sistema de arquivos EXT2
 * @param cwd Ponteiro para o inode do diretório de trabalho atual
 *
 * @return 0 em sucesso, valor negativo em erro
 *
 * A função pode modificar *cwd para comandos que alteram o diretório atual (ex: cd).
 * O primeiro argumento (argv[0]) sempre contém o nome do comando.
 */
typedef int (*command_fn)(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Estrutura que associa nomes de comandos com suas funções implementadoras.
 *
 * Esta estrutura é usada em tabelas de comandos para mapear strings
 * digitadas pelo usuário para as funções correspondentes.
 */
struct command_entry
{
    const char *name;   /**< Nome do comando (ex: "ls", "cd", "mkdir") */
    command_fn handler; /**< Ponteiro para função que implementa o comando */
};

/*
 * ============================================================================
 * FILESYSTEM INFORMATION AND DIAGNOSTIC COMMANDS
 * ============================================================================
 */

/**
 * Exibe informações detalhadas sobre o sistema de arquivos EXT2.
 *
 * Mostra dados do superbloco como:
 * - Número total de inodes e blocos
 * - Tamanho dos blocos e inodes
 * - Contadores de recursos livres
 * - Informações dos grupos de blocos
 *
 * Uso: info
 */
int cmd_info(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Exibe o conteúdo de um arquivo texto.
 *
 * Lê e imprime na tela o conteúdo completo de um arquivo regular.
 * Similar ao comando 'cat' do Unix.
 *
 * Uso: cat <arquivo>
 *
 * @note Apenas funciona com arquivos regulares (não diretórios)
 */
int cmd_cat(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Exibe atributos detalhados de um arquivo ou diretório.
 *
 * Mostra informações do inode como:
 * - Permissões e tipo de arquivo
 * - Tamanho e timestamps
 * - Número de links
 * - Blocos utilizados
 *
 * Uso: attr <arquivo_ou_diretório>
 */
int cmd_attr(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * DIRECTORY NAVIGATION COMMANDS
 * ============================================================================
 */

/**
 * Altera o diretório de trabalho atual.
 *
 * Navega para um diretório especificado, alterando o contexto
 * de trabalho para operações subsequentes.
 *
 * Uso: cd <diretório>
 *      cd          (vai para raiz)
 *      cd ..       (vai para diretório pai)
 *
 * @note Modifica o valor apontado por *cwd
 */
int cmd_cd(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Lista o conteúdo de um diretório.
 *
 * Exibe arquivos e subdiretórios com formatação colorida:
 * - Diretórios em azul
 * - Arquivos regulares em verde
 *
 * Uso: ls [diretório]
 *      ls          (lista diretório atual)
 *
 * @note Se nenhum argumento for fornecido, lista o diretório atual
 */
int cmd_ls(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Exibe o caminho absoluto do diretório de trabalho atual.
 *
 * Mostra o caminho completo desde a raiz até o diretório atual.
 * Similar ao comando 'pwd' do Unix.
 *
 * Uso: pwd
 */
int cmd_pwd(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * FILE AND DIRECTORY CREATION COMMANDS
 * ============================================================================
 */

/**
 * Cria um novo arquivo vazio ou atualiza timestamp de arquivo existente.
 *
 * Se o arquivo não existir, cria um novo arquivo regular vazio.
 * Se existir, apenas atualiza os timestamps de acesso e modificação.
 *
 * Uso: touch <nome_do_arquivo>
 *
 * @note Similar ao comando 'touch' do Unix
 */
int cmd_touch(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Cria um novo diretório.
 *
 * Cria um diretório com as entradas padrão '.' e '..' já configuradas.
 * O diretório pai deve existir e ter permissões de escrita.
 *
 * Uso: mkdir <nome_do_diretório>
 *
 * @note Não cria diretórios intermediários (não é mkdir -p)
 */
int cmd_mkdir(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * FILE AND DIRECTORY REMOVAL COMMANDS
 * ============================================================================
 */

/**
 * Remove um arquivo regular.
 *
 * Deleta um arquivo do sistema, liberando seu inode e blocos de dados.
 * Não funciona com diretórios (use rmdir para isso).
 *
 * Uso: rm <nome_do_arquivo>
 *
 * @note Operação irreversível - dados são perdidos permanentemente
 */
int cmd_rm(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Remove um diretório vazio.
 *
 * Deleta um diretório que contenha apenas as entradas '.' e '..'.
 * Falha se o diretório contiver outros arquivos ou subdiretórios.
 *
 * Uso: rmdir <nome_do_diretório>
 *
 * @note Diretório deve estar vazio para ser removido
 */
int cmd_rmdir(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * FILE MANIPULATION COMMANDS
 * ============================================================================
 */

/**
 * Renomeia ou move um arquivo/diretório.
 *
 * Altera o nome de um arquivo ou diretório dentro do mesmo
 * diretório pai. Funcionalmente equivale a mover o arquivo.
 *
 * Uso: rename <nome_atual> <nome_novo>
 *      mv <nome_atual> <nome_novo>
 *
 * @note O arquivo de destino não deve existir
 */
int cmd_rename(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/**
 * Copia um arquivo para outro local.
 *
 * Cria uma cópia completa de um arquivo regular, incluindo
 * conteúdo e metadados básicos.
 *
 * Uso: cp <arquivo_origem> <arquivo_destino>
 *
 * @note Funciona apenas com arquivos regulares (não diretórios)
 * @note Sobrescreve arquivo de destino se já existir
 */
int cmd_cp(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * DEBUG AND UTILITY COMMANDS
 * ============================================================================
 */

/**
 * Imprime informações de debug sobre uma entrada de diretório.
 *
 * Exibe detalhes técnicos de baixo nível sobre uma entrada específica,
 * incluindo estrutura interna e metadados.
 *
 * Uso: print <arquivo_ou_diretório>
 *
 * @note Comando principalmente para debug e análise técnica
 */
int cmd_print(int argc, char **argv, ext2_fs_t *fs, uint32_t *cwd);

/*
 * ============================================================================
 * COMMAND TABLE UTILITIES
 * ============================================================================
 */

/**
 * Marcador de fim de tabela de comandos.
 *
 * Esta macro deve ser usada como último elemento em arrays de
 * struct command_entry para indicar o final da tabela.
 *
 * Exemplo de uso:
 * struct command_entry commands[] = {
 *     {"ls", cmd_ls},
 *     {"cd", cmd_cd},
 *     CMD_TABLE_END
 * };
 */
#define CMD_TABLE_END {NULL, NULL}

#endif /* COMMANDS_H */
