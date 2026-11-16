# Projeto: Ringue de Aço (jogo em C com raylib)

## Contexto
Projeto da disciplina Algoritmos e Estruturas de Dados (CESAR School, turma B), tema "Ode ao Cinema". Jogo inspirado no filme Gigantes de Aço (Real Steel, 2011). Grupo: Paulo Portella, Gustavo Laporte e Gabriel Ferraz. A especificação já foi entregue à professora, e o grupo vai defender o código numa apresentação, então o código precisa seguir as estruturas, o algoritmo e os nomes de funções declarados nela. Se alguma regra abaixo for inviável ou contraditória, pare e pergunte antes de mudar.

## Resumo do jogo
Duelo PvP local por turnos, com 2 jogadores no mesmo computador. Cada um comanda um robô feito de peças. A cada rodada os dois programam em segredo até 3 ações. Depois, as ações dos dois são unidas numa lista, ordenadas por velocidade e executadas uma a uma. Uma luta dura poucos minutos e termina com opção de revanche imediata.

## Requisitos técnicos obrigatórios
- Linguagem C (C99), compilada com gcc via Makefile (make, make run, make test, make clean).
- Interface gráfica 2D com raylib 5.x. Nenhuma biblioteca além da padrão do C e da raylib.
- Nenhuma IA nem API externa. Jogo 100% determinístico (sem rand na resolução).
- Árvore binária e lista simplesmente encadeada implementadas à mão com malloc/free. Não usar vetores no lugar delas nessas funções e não usar qsort.
- Ordenação: Insertion Sort implementado sobre a lista encadeada, reencadeando ponteiros (não trocar valores entre nós).
- Toda memória liberada (liberarRobo em pós-ordem, liberarLista), sem vazamentos (checar com valgrind se disponível).
- Lógica separada da interface: robo.c, acoes.c e batalha.c não incluem raylib.h, para a lógica ser testável no terminal.
- Identificadores, comentários e textos do jogo em português do Brasil. Comentar cada função dizendo o que faz e a complexidade das operações principais (busca na árvore, desativar subárvore, Insertion Sort).
- Sem imagens, nomes de personagens ou logos do filme, porque o jogo vai para uma mostra pública. Robôs desenhados com formas da raylib.

## Estrutura de arquivos
```
ringue-de-aco/
  Makefile
  README.md
  src/
    main.c          laço principal e máquina de telas
    config.h        todas as constantes de balanceamento
    robo.h/.c       árvore binária de peças
    acoes.h/.c      lista encadeada de ações e Insertion Sort
    batalha.h/.c    regras: validação, resolução, vencedor
    interface.h/.c  desenho e entrada (único módulo com raylib)
  tests/
    test_logica.c   testes com assert da árvore, lista, ordenação e resolução
```

## Estrutura 1: árvore binária (o robô)
Formato fixo, 9 nós:
```
               Núcleo
             /        \
         Tronco        Base
         /    \        /    \
    BraçoE  BraçoD  PernaE  PernaD
      |        |
   PunhoE   PunhoD    (filho esquerdo de cada braço)
```
Struct sugerida:
```c
typedef enum { NUCLEO, TRONCO, BASE, BRACO_E, BRACO_D, PUNHO_E, PUNHO_D, PERNA_E, PERNA_D, NUM_PECAS } TipoPeca;

typedef struct Peca {
    char nome[16];
    TipoPeca tipo;
    int resistencia, resistenciaMax;
    bool ativa;
    struct Peca *esq, *dir;
} Peca;
```
Regras:
- aplicarDano reduz a resistência (mínimo 0). Ao zerar, chama desativarSubarvore na peça: ela e todos os descendentes ficam inativos. Dano excedente é perdido.
- Consequências: Tronco destruído desliga os dois braços e punhos (o robô não ataca mais). Braço destruído desliga o punho daquele lado. Base destruída desliga as pernas, e o robô fica sem esquiva e com modificador de velocidade 0.
- pecaExposta: o Núcleo só pode ser atingido se o Tronco ou a Base estiverem inativos. As demais peças podem ser atingidas se estiverem ativas.
- calcularIntegridade: percorre a árvore somando a resistência das peças ativas.
- velocidadeDoRobo: +1 por perna ativa (0 a 2), ou 0 se a Base estiver inativa.
- Resistências iniciais (config.h): Núcleo 60, Tronco 80, Base 70, cada Braço 40, cada Punho 30, cada Perna 35. Total de 420.

## Estrutura 2: lista simplesmente encadeada (as ações)
```c
typedef enum { JAB, DIRETO, GANCHO, BLOQUEIO, ESQUIVA } TipoAcao;

typedef struct Acao {
    int jogador;            // 1 ou 2
    TipoAcao tipo;
    int lado;               // punho usado: 0 esquerdo, 1 direito (só golpes)
    TipoPeca alvo;          // peça do adversário (só golpes)
    int posicao;            // 0, 1 ou 2 na lista do jogador
    int velocidadeEfetiva;
    int ordemIntercalacao;  // critério de desempate
    struct Acao *prox;
} Acao;
```
Tabela de ações (config.h):

| Ação | Energia | Velocidade base | Dano | Requisito e efeito |
|---|---|---|---|---|
| Jab | 1 | 8 | 10 | punho do lado ativo |
| Direto | 2 | 5 | 20 | punho do lado ativo |
| Gancho | 3 | 3 | 35 | punho do lado ativo; ignora o Bloqueio |
| Bloqueio | 1 | 9 | 0 | depois de executar, os golpes recebidos na rodada causam metade do dano (exceto Gancho) |
| Esquiva | 2 | 6 | 0 | exige Base ativa; depois de executar, anula o próximo golpe recebido na rodada |

- Cada jogador tem 5 de energia por rodada e programa de 1 a 3 ações sem passar desse limite.
- velocidadeEfetiva = velocidade base + bônus de posição (1ª ação +2, 2ª +1, 3ª +0) + velocidadeDoRobo. O bônus de posição faz a ordem montada pelo jogador importar, por isso inserir, remover e trocar ações na lista são parte da estratégia.
- Triângulo resultante (explicar no README): Bloqueio segura Jab e Direto, Gancho quebra o Bloqueio, Esquiva pega o Gancho porque ele é lento.

## Ordenação: Insertion Sort por velocidade
- unirListas intercala as duas listas movendo os nós, sem copiar (J1a1, J2a1, J1a2, J2a2...). Nas rodadas ímpares o J1 começa a intercalação e nas pares o J2. O campo ordemIntercalacao guarda essa posição.
- ordenarPorVelocidade: Insertion Sort na lista encadeada, em ordem decrescente de velocidadeEfetiva. Empate decidido por ordemIntercalacao crescente (ordenação estável).
- Resolução: pega o primeiro nó, executa, remove e libera. Depois recalcula a velocidadeEfetiva das ações restantes (as pernas podem ter sido danificadas), chama removerAcoesInvalidas e reordena. Repete até a lista esvaziar ou um núcleo cair.
- removerAcoesInvalidas remove golpes cujo punho está inativo e esquivas com Base inativa. A ação removida aparece na tela como cancelada (por exemplo: "Braço destruído, golpe cancelado").
- Golpe cujo alvo ficou inativo ou deixou de estar exposto durante a rodada não causa dano ("golpe no vazio").

## Fim de luta
- Núcleo destruído: vitória imediata de quem destruiu. Verificar após cada ação executada.
- Ao fim de 10 rodadas, vence quem tiver maior integridade. Se der igual, é empate.
- Placar da sessão (J1 x J2) mantido enquanto o programa estiver aberto.

## Funções obrigatórias
Estes nomes já foram entregues na especificação. Manter exatamente estes nomes, ajustando tipos de parâmetros se precisar. Funções auxiliares extras são permitidas.
- Robô: criarPeca, inserirPeca, montarRobo, buscarPeca, aplicarDano, desativarSubarvore, pecaExposta, calcularIntegridade, velocidadeDoRobo, desenharRobo, liberarRobo.
- Ações: criarAcao, inserirAcao, removerAcao, trocarAcoes, unirListas, ordenarPorVelocidade, removerAcoesInvalidas, liberarLista.
- Fluxo: menuPrincipal, telaProgramacao, resolverRodada, verificarVencedor.

buscarPeca deve ser uma busca recursiva na árvore (pré-ordem), não um acesso por vetor. desenharRobo percorre a árvore. desenharRobo, menuPrincipal e telaProgramacao ficam em interface.c.

## Telas e fluxo (janela 1280x720, 60 FPS)
1. Menu: Nova luta, Como jogar, Sair.
2. Como jogar: regras resumidas em uma tela, com a tabela de ações.
3. Programação do J1 (tela secreta): mostra os dois robôs com o estado público, a energia restante e a lista do J1 com 3 espaços. O jogador escolhe a ação e o lado e clica na peça-alvo do robô rival (só as peças atingíveis ficam clicáveis). Pode inserir numa posição, remover e trocar ações de lugar, com mouse e teclado. Confirma com ENTER.
4. Tela de troca: "Passe para o JOGADOR 2", escondendo tudo; ESPAÇO continua. O mesmo vale antes do J1 na rodada seguinte.
5. Programação do J2.
6. Resolução: no topo, a "fila de execução" mostra a lista ordenada em cartões (jogador, ação, velocidade). Cada ação executa com uma animação curta (o punho avança, a peça atingida pisca, o dano aparece em texto) de cerca de 1 s, ou ESPAÇO para adiantar. Ações canceladas aparecem riscadas. A fila se reordena de forma visível quando a velocidade muda.
7. Resumo da rodada e próxima rodada.
8. Fim: vencedor, integridade final e placar. R inicia a revanche direto, sem passar pelo menu; M volta ao menu.

- F1 a qualquer momento da luta abre uma sobreposição de depuração: a árvore de cada robô desenhada como diagrama (nós, arestas e resistência) e a lista atual com setas. Será usada na apresentação.
- Visual: vista lateral 2D, com o robô do J1 vermelho à esquerda e o do J2 azul à direita, um de frente para o outro, peças feitas de retângulos e círculos. A cor de cada peça indica a vida (verde, amarelo, vermelho) e fica cinza quando a peça está inativa. Na tela: barra de integridade, energia e "Rodada x/10".

## Ordem de trabalho
- Fase 1: robo, acoes, batalha e tests/test_logica.c rodando com make test, sem raylib. Testes mínimos: cascata de desativação, núcleo exposto, integridade, Insertion Sort com empate estável, remoção de ação inválida e uma luta completa programada até a vitória.
- Fase 2: interface com raylib e todas as telas.
- Fase 3: polimento visual, README e checagem de vazamentos.

Ao fim de cada fase, compile, rode os testes e faça um commit com mensagem em português antes de seguir.

## README
- Como instalar a raylib e compilar no Linux, no macOS e no Windows (MinGW/w64devkit).
- Como jogar.
- Tabela ligando cada estrutura e o Insertion Sort às funções e arquivos do código.
- Integrantes e referências.

## Fora do escopo
Campanha, loja, montagem de robô, adversário controlado pelo computador, rede, salvamento e som obrigatório.
