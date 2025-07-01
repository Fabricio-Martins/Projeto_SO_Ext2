# Projeto Sistemas Operacionais – Implementação de Sistema de Arquivos EXT2

Implemente estruturas de dados e operações para manipular a imagem (.iso) de um sistema de arquivos EXT2.

As operações deverão ser invocadas a partir de um prompt (shell).

O shell deve executar as operações a partir da referência do diretório corrente.

Considere que o programa shell desenvolvido sempre inicia no raiz (/) da imagem manipulada

---

Operações:

- `info`: exibe informações do disco e do sistema de arquivos.
- `cat <filename>`: exibe o conteúdo de um arquivo no formato texto.
- `attr <file | dir>`: exibe os atributos de um arquivo (_file_) ou diretório (_dir_).
- `cd <path>`: altera o diretório corrente para o definido como _path_.
- `ls`: listar os arquivos e diretórios do diretório corrente.
- `pwd`: exibe o diretório corrente (caminho absoluto).
- `touch <filename>`: cria o arquivo _filename_ com conteúdo vazio.
- `mkdir <dir>`: cria o diretório _dir_ vazio.
- `rm <filename>`: remove o arquivo _filename_, se existir.
- `rmdir <dir>`: remove o diretório _dir_, se existir e estiver vazio.
- `rename <filename> <newfilename>` : renomeia arquivo _filename_ para _newfilename_.
- `cp <source_path> <target_path>`: copia um arquivo de origem (_source_path_) para destino (_target_path_). A origem e o destino devem ser uma partição do disco e o volume ext2 ou vice-versa.
- `echo <filename> text`: substitui o conteúdo de um arquivo existente com o valor de _text_.
- `print [ superblock | groups | inode | rootdir | dir | inodebitmap | blockbitmap | attr | block ]`: exibe informações do sistema ext2.

As operações de (1) a (6) envolvem somente a leitura da imagem.

As operações de (7) a (11) envolvem a escrita na imagem.

As operações (12) e (13), cp e mv, copiam e movem arquivos entre o sistema arquivos da partição atual e o sistema de
arquivos da imagem.
