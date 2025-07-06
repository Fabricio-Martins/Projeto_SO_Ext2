/**
 * ============================================================================
 * EXT2 FILESYSTEM SHELL - MAIN PROGRAM
 * ============================================================================
 *
 * Este programa implementa um shell interativo para navegação e manipulação
 * de sistemas de arquivos EXT2. Permite ao usuário executar comandos similares
 * ao shell Unix para explorar imagens de disco EXT2.
 *
 * O programa carrega uma imagem EXT2 e oferece uma interface de linha de comando
 * com comandos como ls, cd, mkdir, rm, cat, etc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "ops/ext2-operations.h"

/*
 * ============================================================================
 * SHELL CONFIGURATION CONSTANTS
 * ============================================================================
 */

/** Número máximo de tokens (argumentos) em uma linha de comando */
#define MAX_TOKENS 32

/** Tamanho máximo do buffer para entrada de linha de comando */
#define MAX_BUFFER_SHELL 1024

/*
 * ============================================================================
 * COMMAND LINE PARSING FUNCTIONS
 * ============================================================================
 */

/**
 * Tokeniza uma linha de comando em argumentos individuais.
 *
 * Esta função analisa uma string de entrada e a divide em tokens (palavras)
 * separados por espaços, tabs ou newlines. Suporta strings entre aspas
 * simples ou duplas para argumentos que contenham espaços.
 *
 * Funcionalidades:
 * - Ignora espaços em branco múltiplos entre tokens
 * - Suporta aspas simples (') e duplas (") para agrupar argumentos
 * - Remove as aspas do token final
 * - Termina cada token com '\0' para uso como string C
 *
 * @param line String de entrada a ser tokenizada (será modificada)
 * @param argv Array de ponteiros onde os tokens serão armazenados
 * @return Número de tokens encontrados (argc equivalente)
 *
 * @note A string 'line' é modificada durante o processo (tokens são null-terminated)
 * @note O array 'argv' deve ter pelo menos MAX_TOKENS elementos
 * @note O último elemento de 'argv' é sempre definido como NULL
 *
 */
int tokenize(char *line, char **argv)
{
    int argc = 0;
    char *p = line;

    while (*p)
    {
        // Pula espaços em branco fora de aspas (espaços, tabs, newlines)
        while (*p == ' ' || *p == '\t' || *p == '\n')
            ++p;

        // Se chegou ao final da string, termina o processamento
        if (!*p)
            break;

        // Marca o início do token atual
        char *token = p;
        char quote = 0; // Caractere de aspas ativo (0 = sem aspas)

        // Verifica se o token começa com aspas
        if (*p == '"' || *p == '\'')
        {
            // Token entre aspas - salva o tipo de aspas e avança
            quote = *p++;
            token = p; // Conteúdo real do token (sem as aspas iniciais)

            // Procura pela aspa de fechamento correspondente
            while (*p && *p != quote)
                ++p;
        }
        else
        {
            // Token sem aspas - procura por próximo espaço ou fim da linha
            while (*p && *p != ' ' && *p != '\t' && *p != '\n')
                ++p;
        }

        // Termina o token atual com null terminator
        if (*p)
            *p++ = '\0';

        // Armazena o ponteiro para o token no array de argumentos
        argv[argc++] = token;

        // Previne overflow do array de argumentos
        if (argc >= MAX_TOKENS - 1)
            break;
    }

    // Marca o final da lista de argumentos (convenção padrão argc/argv)
    argv[argc] = NULL;
    return argc;
}

/*
 * ============================================================================
 * COMMAND TABLE DEFINITION
 * ============================================================================
 */

/**
 * Tabela de comandos disponíveis no shell.
 *
 * Esta estrutura mapeia nomes de comandos digitados pelo usuário
 * para suas respectivas funções implementadoras. A tabela é terminada
 * pelo marcador CMD_TABLE_END.
 */
struct command_entry cmd_table[] = {
    {"info", cmd_info},     /* Informações do filesystem */
    {"ls", cmd_ls},         /* Lista conteúdo de diretório */
    {"cd", cmd_cd},         /* Muda diretório atual */
    {"pwd", cmd_pwd},       /* Mostra diretório atual */
    {"cat", cmd_cat},       /* Exibe conteúdo de arquivo */
    {"attr", cmd_attr},     /* Mostra atributos de arquivo */
    {"touch", cmd_touch},   /* Cria arquivo vazio */
    {"mkdir", cmd_mkdir},   /* Cria diretório */
    {"rm", cmd_rm},         /* Remove arquivo */
    {"rmdir", cmd_rmdir},   /* Remove diretório vazio */
    {"rename", cmd_rename}, /* Renomeia arquivo/diretório */
    {"cp", cmd_cp},         /* Copia arquivo */
    {"print", cmd_print},   /* Debug: imprime entrada de diretório */
    CMD_TABLE_END           /* Marcador de fim de tabela */
};

/*
 * ============================================================================
 * MAIN PROGRAM FUNCTION
 * ============================================================================
 */

/**
 * Função principal do shell EXT2.
 *
 * Implementa o loop principal do shell interativo:
 * 1. Valida argumentos da linha de comando
 * 2. Abre a imagem do sistema de arquivos EXT2
 * 3. Inicializa o ambiente do shell (diretório atual = raiz)
 * 4. Loop principal:
 *    - Exibe prompt com caminho atual
 *    - Lê comando do usuário
 *    - Tokeniza a entrada
 *    - Procura e executa o comando na tabela
 *    - Trata erros e comandos especiais (exit/quit)
 * 5. Limpa recursos e encerra
 *
 * @param argc Número de argumentos da linha de comando
 * @param argv Array de argumentos da linha de comando
 * @return EXIT_SUCCESS em execução normal, EXIT_FAILURE em erro
 *
 */
int main(int argc, char **argv)
{
    // Valida número de argumentos da linha de comando
    if (argc != 2)
    {
        return EXIT_FAILURE;
    }

    // Tenta abrir a imagem do sistema de arquivos EXT2
    ext2_fs_t *fs = fs_open(argv[1]);
    if (!fs)
    {
        fprintf(stderr, "Erro ao abrir a imagem '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    // Inicializa o diretório de trabalho atual como raiz
    uint32_t cwd = EXT2_ROOT_INO;

    // Buffer para armazenar a linha de comando digitada pelo usuário
    char line[MAX_BUFFER_SHELL];

    // Loop principal do shell
    while (1)
    {
        // Obtém e exibe o caminho atual no prompt
        char *pwd = fs_get_path(fs, cwd);
        printf("ext2shell:[%s]$ ", pwd ? pwd : "/");
        free(pwd); // Libera a string do caminho (alocada dinamicamente)

        // Garante que o prompt seja exibido imediatamente
        fflush(stdout);

        // Lê a linha de comando do usuário
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            // EOF detectado (Ctrl+D) ou erro de leitura
            putchar('\n');
            break;
        }

        // Tokeniza a linha de entrada em argumentos
        char *argvv[MAX_TOKENS];
        int argc_cmd = tokenize(line, argvv);

        // Ignora linhas vazias ou que contenham apenas espaços
        if (argc_cmd == 0)
            continue;

        // Trata comandos especiais de saída
        if (strcmp(argvv[0], "exit") == 0 || strcmp(argvv[0], "quit") == 0)
        {
            break;
        }

        // Procura o comando digitado na tabela de comandos disponíveis
        struct command_entry *cmd = NULL;
        for (struct command_entry *ce = cmd_table; ce->name; ++ce)
        {
            if (strcmp(ce->name, argvv[0]) == 0)
            {
                cmd = ce;
                break;
            }
        }

        // Verifica se o comando foi encontrado
        if (!cmd)
        {
            printf("command not found.\n");
            continue;
        }

        // Executa o comando encontrado
        int result = cmd->handler(argc_cmd, argvv, fs, &cwd);
        if (result != 0)
        {
            fprintf(stderr, "Erro ao executar comando '%s': %s\n",
                    cmd->name, strerror(errno));

            // Reset errno para próxima operação
            errno = 0;
        }
    }

    // Limpa recursos antes de encerrar
    fs_close(fs);
    return EXIT_SUCCESS;
}