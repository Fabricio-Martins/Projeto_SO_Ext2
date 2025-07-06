# Projeto EXT2 - Sistemas Operacionais

## Descrição

Este projeto implementa funcionalidades do sistema de arquivos EXT2 como parte da disciplina de Sistemas Operacionais.

## Objetivos

- Compreender a estrutura do sistema de arquivos EXT2
- Implementar operações básicas de manipulação de arquivos
- Aplicar conceitos de sistemas operacionais na prática

## Funcionalidades Implementadas

### Operações a serem implementadas

1. **info**: Exibe informações do disco e do sistema de arquivos.
2. **cat &lt;file&gt;**: Exibe o conteúdo de um arquivo em formato texto.
3. **attr &lt;file | dir&gt;**: Exibe os atributos de um arquivo (`file`) ou diretório (`dir`).
4. **cd &lt;path&gt;**: Altera o diretório corrente para o definido em `path`.
5. **ls**: Lista os arquivos e diretórios do diretório corrente.
6. **pwd**: Exibe o diretório corrente (caminho absoluto).
7. **touch &lt;file&gt;**: Cria o arquivo `file` com conteúdo vazio.
8. **mkdir &lt;dir&gt;**: Cria o diretório `dir` vazio.
9. **rm &lt;file&gt;**: Remove o arquivo `file` do sistema.
10. **rmdir &lt;dir&gt;**: Remove o diretório `dir`, se estiver vazio.
11. **rename &lt;file&gt; &lt;newfilename&gt;**: Renomeia o arquivo `file` para `newfilename`.
12. **cp &lt;source_path&gt; &lt;target_path&gt;**: Copia um arquivo de origem (`source_path`) para destino (`target_path`).

## Como Compilar

```bash
make
```

## Como Executar

```bash
./main <imagem.img>
```

## Comandos e Estrutura do Volume `myext2image.img`

### Gerando Imagens EXT2 (64MiB com blocos de 1K)

```bash
dd if=/dev/zero of=./myext2image.img bs=1024 count=64K
mkfs.ext2 -b 1024 ./myext2image.img
```

### Verificando a Integridade do Sistema EXT2

```bash
e2fsck myext2image.img
```

### Montando a Imagem do Volume com EXT2

```bash
sudo mount myext2image.img /mnt
```

### Estrutura Original de Arquivos do Volume

Comando utilizado: `tree`

### Informações de Espaço

Comando utilizado: `df`

- **Blocos de 1K:** 62186
- **Usado:** 26777 KiB
- **Disponível:** 32133 KiB

### Desmontando a Imagem do Volume com EXT2

```bash
sudo umount /mnt
```
