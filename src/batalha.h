/*
 * batalha.h
 * Regras do duelo: validação da programação, resolução da rodada passo a
 * passo e verificação do vencedor.
 *
 * Este módulo não usa raylib: pode ser testado no terminal.
 */
#ifndef BATALHA_H
#define BATALHA_H

#include <stdbool.h>
#include "robo.h"
#include "acoes.h"

#define EMPATE       3     /* valor de vencedor quando a luta empata */
#define MAX_LOG      24    /* linhas de registro guardadas por rodada */
#define TAM_TEXTO    128

/* O que aconteceu na última ação executada (a interface usa para animar) */
typedef struct {
    bool executou;          /* false se não havia ação para executar */
    Acao acao;              /* cópia da ação executada (prox = NULL) */
    int danoCausado;        /* dano realmente aplicado */
    bool noVazio;           /* alvo inativo ou não exposto */
    bool esquivado;         /* anulado por Esquiva */
    bool bloqueado;         /* dano pela metade por Bloqueio */
    bool pecaDestruida;     /* o golpe zerou a peça */
    int canceladasNoPasso;  /* ações canceladas logo após este passo */
    char texto[TAM_TEXTO];
    char textoCancelamento[TAM_TEXTO];
} Evento;

typedef struct {
    Peca *robo[2];             /* robo[0] = J1, robo[1] = J2 */
    Acao *programacao[2];      /* lista de cada jogador durante a programação */
    Acao *fila;                /* lista unida e ordenada durante a resolução */
    Acao *canceladas;          /* ações canceladas nesta rodada */
    int rodada;                /* 1 a MAX_RODADAS */
    bool bloqueioAtivo[2];     /* Bloqueio já executado nesta rodada */
    int esquivasPendentes[2];  /* Esquivas executadas e ainda não usadas */
    int vencedor;              /* 0 em andamento, 1, 2 ou EMPATE */
    bool vitoriaPorNucleo;     /* true se acabou por núcleo destruído */
    char registro[MAX_LOG][TAM_TEXTO];  /* textos da rodada para o resumo */
    int numRegistros;
} Batalha;

/* Início e fim de uma luta */
bool iniciarBatalha(Batalha *b);
void encerrarBatalha(Batalha *b);

/* Programação */
int  energiaRestante(const Batalha *b, int jogador);
int  velocidadePrevista(Batalha *b, int jogador, TipoAcao tipo, int posicao);
bool validarAcao(Batalha *b, int jogador, TipoAcao tipo, int lado, TipoPeca alvo,
                 const char **motivo);
bool programarAcao(Batalha *b, int jogador, TipoAcao tipo, int lado, TipoPeca alvo,
                   int posicao, const char **motivo);

/* Resolução */
void prepararResolucao(Batalha *b);
bool executarProximaAcao(Batalha *b, Evento *ev);
void resolverRodada(Batalha *b);
void encerrarRodada(Batalha *b);
void atualizarVelocidades(Batalha *b);

/* Fim de luta */
int  verificarVencedor(Batalha *b, bool fimDaRodada);

#endif /* BATALHA_H */
