# Ringue de Aço

Duelo PvP local por turnos entre dois robôs, feito em C99 com raylib.
Projeto da disciplina **Algoritmos e Estruturas de Dados** (CESAR School, turma B), tema "Ode ao Cinema".

Dois jogadores no mesmo computador. A cada rodada, cada um programa em segredo até 3 ações. As ações dos dois são unidas numa lista encadeada, ordenadas por velocidade com **Insertion Sort** e executadas uma a uma. Cada robô é uma **árvore binária** de peças: quando uma peça é destruída, tudo que está abaixo dela desliga.

## Como executar (resumo)

1. Tenha `gcc`, `make` e a raylib 5.5 (a 6 também funciona) instalados (passo a passo por sistema logo abaixo).
2. Na pasta do projeto, rode:

```bash
make run
```

Se a raylib não estiver num caminho padrão, informe a pasta dela (a que contém `include/` e `lib/`):

```bash
make RAYLIB=C:/raylib run
```

3. No menu, escolha **Nova luta**. O Jogador 1 programa as ações e confirma com ENTER, passa o computador para o Jogador 2, e a rodada é resolvida na tela. As regras completas estão em [Como jogar](#como-jogar) e na opção **Como jogar** do menu.

Para rodar só os testes da lógica (não precisa de raylib): `make test`.

---

## Como compilar e rodar

Comandos do Makefile:

| Comando | O que faz |
|---|---|
| `make` | compila o jogo em `build/` |
| `make run` | compila e abre o jogo |
| `make test` | compila e roda os testes da lógica (não precisa de raylib) |
| `make valgrind` | roda os testes no valgrind (Linux) |
| `make clean` | apaga `build/` |

O Makefile procura a raylib assim:
1. se você passar `RAYLIB=<pasta>` (a pasta que contém `include/` e `lib/`), usa essa pasta;
2. senão, se existir `pkg-config` com a raylib, usa o `pkg-config`;
3. senão, tenta `-lraylib` nos caminhos padrão do compilador.

Para não digitar `RAYLIB=...` toda vez, crie um arquivo `local.mk` na raiz (ele não vai para o git):

```make
RAYLIB = C:/raylib
```

### Windows (w64devkit)

1. Baixe o **w64devkit** (gcc + make) em https://github.com/skeeto/w64devkit/releases e extraia, por exemplo em `C:\w64devkit`.
2. Baixe a raylib 5.5 pré-compilada para MinGW: `raylib-5.5_win64_mingw-w64.zip` em https://github.com/raysan5/raylib/releases/tag/5.5 e extraia.
   **Atenção:** o zip tem uma pasta dentro (`raylib-5.5_win64_mingw-w64`). O `RAYLIB=` precisa apontar para a pasta que contém `include` e `lib` diretamente. O jeito mais simples: mova o conteúdo dessa pasta para `C:\raylib`, de modo que existam `C:\raylib\include\raylib.h` e `C:\raylib\lib\libraylib.a`.
3. Abra o `C:\w64devkit\w64devkit.exe` (ou coloque `C:\w64devkit\bin` no PATH), entre na pasta do projeto e rode:

```bash
make RAYLIB=C:/raylib run
```

Use barras normais (`/`) no caminho. Se a raylib ficou em outra pasta, troque `C:/raylib` pelo caminho dela.

Se o Windows mostrar "Uma política de Controle de Aplicativo bloqueou este arquivo", é o **Controle Inteligente de Aplicativos** do Windows 11 barrando um executável novo e sem assinatura. Rode o jogo em outra máquina ou ajuste essa configuração em Segurança do Windows > Controle de aplicativos e navegador (a decisão é sua; o jogo não precisa disso para compilar nem para os testes).

### Linux (Ubuntu/Debian)

Compile e instale a raylib 5.5 a partir do código-fonte:

```bash
sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

```bash
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
```

```bash
cd raylib/src && make PLATFORM=PLATFORM_DESKTOP && sudo make install
```

Depois, na pasta do projeto:

```bash
make run
```

Outra opção é extrair `raylib-5.5_linux_amd64.tar.gz` das releases e usar `make RAYLIB=/caminho/da/raylib run` (se ligar com a `.so`, exporte `LD_LIBRARY_PATH=/caminho/da/raylib/lib`).

### macOS

Instale as ferramentas de linha de comando da Apple (trazem o compilador e o `make`) e o [Homebrew](https://brew.sh), se ainda não tiver:

```bash
xcode-select --install
```

Depois instale a raylib e o pkg-config e rode o jogo na pasta do projeto:

```bash
brew install raylib pkg-config
```

```bash
make run
```

No macOS o `gcc` é o clang da Apple, e funciona normalmente. O jogo abre numa janela; o Terminal só serve para iniciar.

---

## Como jogar

**Objetivo:** destruir o **Núcleo** do robô rival. Se ninguém conseguir em 6 rodadas, vence quem tiver mais integridade (soma da resistência das peças ativas). Igual: empate.

**A rodada:**
1. O Jogador 1 programa sua lista em segredo e confirma com ENTER.
2. Tela de troca: "Passe para o JOGADOR 2". ESPAÇO continua.
3. O Jogador 2 programa e confirma.
4. Resolução: as listas são intercaladas (J1 começa nas rodadas ímpares, J2 nas pares), ordenadas por velocidade e executadas uma a uma, com animação. ESPAÇO adianta.
5. Resumo da rodada e próxima rodada.

**Programação (5 de energia, de 1 a 3 ações):**

| Ação | Energia | Velocidade base | Dano | Requisito e efeito |
|---|---|---|---|---|
| Jab | 1 | 8 | 10 | punho do lado escolhido ativo |
| Direto | 2 | 5 | 20 | punho do lado escolhido ativo |
| Gancho | 3 | 3 | 35 | punho ativo; ignora o Bloqueio |
| Bloqueio | 1 | 9 | 0 | depois de executar, golpes recebidos na rodada causam metade do dano (exceto Gancho) |
| Esquiva | 2 | 6 | 0 | exige Base ativa; depois de executar, anula o próximo golpe recebido na rodada |

**Velocidade efetiva** = velocidade base + bônus de posição (1ª ação +2, 2ª +1, 3ª +0) + 1 por perna ativa (0 se a Base caiu). Por isso a ordem em que você monta a lista importa. Empate de velocidade: executa primeiro quem veio antes na intercalação.

**O triângulo:**
- **Bloqueio segura Jab e Direto:** é a ação mais rápida (9), então executa antes deles e corta o dano pela metade.
- **Gancho quebra o Bloqueio:** é o único golpe que ignora a guarda.
- **Esquiva pega o Gancho:** o Gancho é o mais lento (3), então a Esquiva (6) quase sempre executa antes e anula o golpe.

**Peças:** Núcleo 40, Tronco 50, Base 45, cada Braço 25, cada Punho 20, cada Perna 25 (total 275).
- Tronco destruído desliga os dois braços e punhos: o robô não ataca mais.
- Braço destruído desliga o punho daquele lado.
- Base destruída desliga as pernas: sem esquiva e sem bônus de velocidade.
- O Núcleo só pode ser atingido se o Tronco **ou** a Base estiverem destruídos.
- Golpe programado com um punho que caiu durante a rodada é **cancelado** (aparece riscado). Golpe cujo alvo caiu ou deixou de estar exposto vira **golpe no vazio**.

**Controles da programação:**

| Tecla / mouse | Ação |
|---|---|
| 1 a 5 ou clique nos botões | escolher a ação |
| setas esquerda/direita (A/D) | escolher o punho |
| clique numa peça do rival com contorno | programar o golpe nessa peça |
| ESPAÇO ou botão ADICIONAR | programar Bloqueio/Esquiva |
| setas cima/baixo (W/S) ou clique no espaço | escolher a posição da lista |
| SHIFT + cima/baixo ou botões ^ v | trocar a ação de lugar |
| DEL / BACKSPACE ou botão x | remover a ação |
| ENTER | confirmar |
| F1 (durante a luta) | depuração: árvores dos robôs e lista atual com setas |

No fim da luta: **R** revanche direto, **M** menu. O placar vale enquanto o programa estiver aberto.

---

## Estruturas de dados no código

| Conceito | Arquivo | Funções |
|---|---|---|
| Árvore binária do robô (9 nós, `malloc`/`free`) | `src/robo.h`, `src/robo.c` | `criarPeca`, `inserirPeca`, `montarRobo`, `buscarPeca`, `aplicarDano`, `desativarSubarvore`, `pecaExposta`, `calcularIntegridade`, `velocidadeDoRobo`, `liberarRobo` |
| Desenho da árvore | `src/interface.c` | `desenharRobo` (percurso pós-ordem), `desenharNoArvore` (diagrama do F1) |
| Lista simplesmente encadeada de ações | `src/acoes.h`, `src/acoes.c` | `criarAcao`, `inserirAcao`, `removerAcao`, `trocarAcoes`, `unirListas`, `removerAcoesInvalidas`, `retirarPrimeira`, `liberarLista` |
| Insertion Sort na lista | `src/acoes.c` | `ordenarPorVelocidade` (com o critério `vemAntes`) |
| Regras e resolução | `src/batalha.h`, `src/batalha.c` | `validarAcao`, `programarAcao`, `prepararResolucao`, `executarProximaAcao`, `resolverRodada`, `encerrarRodada`, `verificarVencedor` |
| Telas | `src/interface.c`, `src/main.c` | `menuPrincipal`, `telaProgramacao`, `telaResolucao`, `telaResumo`, `telaFim`, máquina de telas em `main.c` |
| Constantes de balanceamento | `src/config.h` | todas as resistências, custos, velocidades e danos |

**Complexidades principais** (n = peças da árvore, m = ações na lista, m ≤ 6):

| Operação | Complexidade | Observação |
|---|---|---|
| `buscarPeca` | O(n) | busca recursiva em pré-ordem |
| `desativarSubarvore` | O(k) | k = nós da subárvore |
| `calcularIntegridade` | O(n) | percurso completo |
| `liberarRobo` | O(n) | pós-ordem: filhos antes do pai |
| `inserirAcao`, `removerAcao`, `trocarAcoes` | O(m) | só reencadeiam ponteiros |
| `unirListas` | O(m) | move os nós, não copia |
| `ordenarPorVelocidade` | O(m²) pior caso, O(m) melhor caso | Insertion Sort estável, memória extra O(1) |

**Como a resolução usa as estruturas:** `prepararResolucao` chama `unirListas` e `ordenarPorVelocidade`. A cada passo, `executarProximaAcao` retira o primeiro nó, executa, libera o nó, recalcula as velocidades (as pernas podem ter caído), chama `removerAcoesInvalidas` e reordena. `resolverRodada` repete esse passo até a fila esvaziar ou um núcleo cair; a interface chama os mesmos passos um a um para poder animar cada ação.

**Lógica separada da interface:** `robo.c`, `acoes.c` e `batalha.c` não incluem `raylib.h`. Por isso `make test` compila e roda sem raylib.

---

## Testes e memória

`tests/test_logica.c` tem 16 testes com `assert`: montagem e busca na árvore, dano mínimo zero, cascata de desativação (Tronco, Braço, Base), núcleo exposto, inserir/remover/trocar ações, intercalação nas rodadas ímpares e pares, Insertion Sort com empate estável (conferindo que os nós mudaram de lugar, não os valores), remoção de ações inválidas, validação, Bloqueio x Gancho, Esquiva, golpe no vazio, cancelamento e reordenação no meio da rodada, uma luta completa até a vitória e o fim por rodadas (empate e vitória por integridade).

A cada push, o GitHub Actions roda `make test` no Linux, no macOS e no Windows, e `make valgrind` no Linux.

No Linux e no Windows, o Makefile liga os testes com `-Wl,--wrap=malloc -Wl,--wrap=free`: cada `malloc` e `free` passa por um contador, e cada teste confere que nenhuma alocação ficou sobrando. No Linux também dá para rodar `make valgrind`.

---

## Estrutura de arquivos

```
Makefile
README.md
src/
  main.c          laço principal e máquina de telas
  config.h        constantes de balanceamento
  robo.h/.c       árvore binária de peças
  acoes.h/.c      lista encadeada de ações e Insertion Sort
  batalha.h/.c    regras: validação, resolução, vencedor
  interface.h/.c  desenho e entrada (único módulo de jogo com raylib)
tests/
  test_logica.c   testes com assert
ferramentas/
  capturar_telas.c  joga uma luta roteirizada e salva um PNG de cada tela
  centros_pecas.c   ponto do meio de cada peça na tela (para clicar no vídeo)
  gravador.c        grava cada quadro do jogo (ligado com -Wl,--wrap=EndDrawing)
  gravar_video.sh   joga uma luta completa numa tela virtual do Linux e grava
pacote/           LEIA-ME e Info.plist dos pacotes gerados pelo GitHub Actions
.github/workflows/
  build.yml       compila e testa no Linux, macOS e Windows
  video.yml       grava o vídeo do jogo (sob demanda)
```

---

## Integrantes

- Paulo Portella
- Gustavo Laporte
- Gabriel Ferraz

## Referências

- raylib 5.5: https://www.raylib.com e https://www.raylib.com/cheatsheet/cheatsheet.html
- w64devkit: https://github.com/skeeto/w64devkit
- CORMEN, T. H. et al. *Algoritmos: Teoria e Prática*. Capítulo 2 (Insertion Sort).
- Inspiração temática: o filme *Gigantes de Aço* (*Real Steel*, 2011). O jogo não usa imagens, nomes de personagens nem logos do filme; os robôs são desenhados com formas da raylib.
