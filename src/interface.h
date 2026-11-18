/*
 * interface.h
 * Desenho e entrada com raylib. É o único módulo que inclui raylib.h
 * (além de main.c, que abre a janela e roda o laço principal).
 */
#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include "raylib.h"
#include "batalha.h"

/* Telas da máquina de estados controlada por main.c */
typedef enum {
    TELA_MENU,
    TELA_COMO_JOGAR,
    TELA_PROGRAMACAO,
    TELA_TROCA,
    TELA_RESOLUCAO,
    TELA_RESUMO,
    TELA_FIM,
    TELA_SAIR
} Tela;

/* Placar da sessão (vale enquanto o programa estiver aberto) */
typedef struct {
    int vitorias[2];
    int empates;
} Placar;

/* Estado da tela de programação de um jogador */
typedef struct {
    TipoAcao acao;          /* ação escolhida nos botões */
    int lado;               /* punho escolhido: 0 E, 1 D */
    int cursor;             /* posição da lista selecionada (0 a 2) */
    bool escolheuAcao;      /* guia: o jogador já escolheu a próxima ação */
    char mensagem[96];      /* aviso de erro */
    float tempoMensagem;    /* segundos restantes do aviso */
} EstadoProgramacao;

/* Efeitos visuais aplicados a um robô ao desenhar */
typedef struct {
    TipoPeca pecaPiscando;      /* NUM_PECAS = nenhuma */
    float piscar;               /* intensidade do pisca (0 a 1) */
    int ladoSoco;               /* -1 nenhum, 0 esquerdo, 1 direito */
    float avancoSoco;           /* quanto o punho avança (pixels sem escala) */
    float recuo;                /* deslocamento para trás (esquiva) */
    bool escudo;                /* Bloqueio ativo */
    bool destacarNucleo;        /* anel pulsando no núcleo exposto */
    TipoPeca pecaRealcada;      /* contorno forte (mouse em cima) */
    bool clicavel[NUM_PECAS];   /* contorno fraco nas peças clicáveis */
    bool pulsarClicaveis;       /* guia: o contorno das clicáveis pisca */
} AnimacaoRobo;

/* Cartão da fila de execução com posição animada */
#define MAX_CARTOES 12
typedef struct {
    const Acao *chave;      /* nó da lista que o cartão representa */
    float x, y;
    int velocidade;
    int velocidadeAnterior;
    float brilho;           /* destaque quando a velocidade muda */
    bool emUso;
} CartaoVisual;

typedef enum { RES_MOSTRANDO_FILA, RES_ANIMANDO, RES_TERMINADA } FaseResolucao;

/* Estado da tela de resolução */
typedef struct {
    FaseResolucao fase;
    float tempo;
    Evento evento;
    CartaoVisual cartoes[MAX_CARTOES];
} EstadoResolucao;

/* Entrada: main.c bloqueia a entrada das telas quando a depuração (F1) está aberta */
void liberarEntrada(bool liberada);

/* Robô */
void     desenharRobo(const Peca *raiz, Vector2 pos, float escala, bool espelhado,
                      Color corTime, const AnimacaoRobo *anim);
TipoPeca pecaNoPonto(Peca *raiz, Vector2 pos, float escala, bool espelhado,
                     Vector2 ponto, bool soExpostas);
void     animacaoPadrao(AnimacaoRobo *anim);

/* Telas */
Tela menuPrincipal(int *opcao);
Tela telaComoJogar(void);
void reiniciarProgramacao(EstadoProgramacao *ep);
Tela telaProgramacao(Batalha *b, int jogador, EstadoProgramacao *ep, const Placar *placar);
Tela telaTroca(int jogador, int rodada);
void reiniciarResolucao(EstadoResolucao *er);
Tela telaResolucao(Batalha *b, EstadoResolucao *er);
Tela telaResumo(Batalha *b, int rodadaEncerrada);
Tela telaFim(Batalha *b, const Placar *placar);

/* Sobreposição de depuração (F1): árvores e lista atual */
void desenharDepuracao(Batalha *b, const Acao *lista, const char *tituloLista,
                       const Acao *canceladas);

#endif /* INTERFACE_H */
