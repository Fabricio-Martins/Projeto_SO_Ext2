## Detalhes de Implementação

- **Linguagem utilizada:** C/C++.

- **Restrições:** Não utilizar chamadas para funções do sistema (ex: `system()`, `exec()`) nem estruturas EXT2 prontas de bibliotecas ou da internet.

- **Simplificações permitidas:**
  - Sintaxe dos comandos pode ser simplificada (ex: não tratar múltiplos diretórios como `rm dir1/dir2/file.txt`).
  - Não há arquivos maiores que 64 MiB.
  - Tamanho fixo de bloco: 1024 bytes.
  - Apenas diretórios que usam 1 bloco para armazenar entradas de diretório são tratados.
- **Limitações:**
  - Não é necessário processar arquivos com ponteiros triplamente indiretos.
  - Escrita de entradas em arquivos de diretório limitada ao tamanho do bloco.
  - Leitura de arquivos de diretório apenas com ponteiros diretos.

## Comandos

Experimente os comandos abaixo no shell interativo:

- [x] **info** — Exibe informações do disco e do sistema de arquivos
- [x] **cat &lt;file&gt;** — Mostra o conteúdo de um arquivo
- [x] **attr &lt;file | dir&gt;** — Exibe atributos de arquivo/diretório
- [x] **cd &lt;path&gt;** — Muda o diretório atual
- [x] **ls** — Lista arquivos e diretórios
- [x] **pwd** — Mostra o caminho absoluto do diretório atual
- [x] **touch &lt;file&gt;** — Cria um arquivo vazio
- [x] **mkdir &lt;dir&gt;** — Cria um diretório vazio
- [x] **rm &lt;file&gt;** — Remove um arquivo
- [x] **rmdir &lt;dir&gt;** — Remove um diretório vazio
- [x] **rename &lt;file&gt; &lt;newfilename&gt;** — Renomeia um arquivo
- [x] **cp &lt;source_path&gt; &lt;target_path&gt;** — Copia arquivo da imagem para o sistema real
- [x] **print [ superblock | groups | inode ]**: exibe informações do sistema EXT2.

> 💡 **Dicas rápidas:**
>
> - Comandos (1) a (6): apenas leitura da imagem.
> - Comandos (7) a (11): escrita na imagem.
> - Comandos (12) e (13): interagem entre a imagem EXT2 e o sistema real (use caminhos absolutos).
> - Comando (14): apenas print da estrutura

---

## Compilação e Execução

1. Compile:

```bash
make
```

2. Execute:

```bash
./main <imagem.img>
```

---

## Comandos e estrutura da imagem

Gerando imagens ext2 (64MiB com blocos de 1K):

```bash
dd if=/dev/zero of=./myext2image.img bs=1024 count=64K
```

```bash
mkfs.ext2 -b 1024 ./myext2image.img
```

Verificando a integridade de um sistema ext2:

```bash
e2fsck myext2image.img
```

Montando a imagem do volume com ext2:

```bash
sudo mount myext2image.img /mnt
```

Estrutura original de arquivos do volume (comando tree via bash)

Desmontando a imagem do volume com ext2:

```bash
sudo umount /mnt
```
