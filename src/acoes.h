/*
 * acoes.h
 * Estrutura 2: lista simplesmente encadeada de ações, e o Insertion Sort
 * que ordena a lista unida dos dois jogadores por velocidade.
 *
 * Este módulo não usa raylib: pode ser testado no terminal.
 */
#ifndef ACOES_H
#define ACOES_H

#include <stdbool.h>
#include "robo.h"

typedef enum { JAB, DIRETO, GANCHO, BLOQUEIO, ESQUIVA, NUM_ACOES } TipoAcao;

typedef struct Acao {
    int jogador;            /* 1 ou 2 */
    TipoAcao tipo;
    int lado;               /* punho usado: 0 esquerdo, 1 direito (só golpes) */
    TipoPeca alvo;          /* peça do adversário (só golpes) */
    int posicao;            /* 0, 1 ou 2 na lista do jogador */
    int velocidadeEfetiva;
    int ordemIntercalacao;  /* critério de desempate */
    struct Acao *prox;
} Acao;

/* Criação e liberação */
Acao *criarAcao(int jogador, TipoAcao tipo, int lado, TipoPeca alvo);
void  liberarLista(Acao *lista);

/* Edição da lista de um jogador (renumeram o campo posicao) */
bool  inserirAcao(Acao **lista, Acao *nova, int posicao);
bool  removerAcao(Acao **lista, int posicao);
bool  trocarAcoes(Acao **lista, int i, int j);
void  renumerarPosicoes(Acao *lista);

/* Resolução */
Acao *unirListas(Acao *listaJ1, Acao *listaJ2, int quemComeca);
void  ordenarPorVelocidade(Acao **lista);
int   removerAcoesInvalidas(Acao **lista, Peca *roboJ1, Peca *roboJ2, Acao **canceladas);
Acao *retirarPrimeira(Acao **lista);

/* Consultas */
int   tamanhoLista(const Acao *lista);
Acao *acaoNaPosicao(Acao *lista, int indice);
int   energiaDaLista(const Acao *lista);
bool  acaoValidaParaRobo(const Acao *a, Peca *roboDono);

/* Tabela de ações (valores de config.h) */
bool        ehGolpe(TipoAcao tipo);
int         custoAcao(TipoAcao tipo);
int         velocidadeBase(TipoAcao tipo);
int         danoAcao(TipoAcao tipo);
int         bonusPosicao(int posicao);
const char *nomeAcao(TipoAcao tipo);

#endif /* ACOES_H */
