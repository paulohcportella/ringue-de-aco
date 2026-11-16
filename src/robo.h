/*
 * robo.h
 * Estrutura 1: o robô é uma árvore binária de peças (9 nós, formato fixo).
 *
 *                Núcleo
 *              /        \
 *          Tronco        Base
 *          /    \        /    \
 *     BraçoE  BraçoD  PernaE  PernaD
 *       |        |
 *    PunhoE   PunhoD    (filho esquerdo de cada braço)
 *
 * Este módulo não usa raylib: pode ser testado no terminal.
 */
#ifndef ROBO_H
#define ROBO_H

#include <stdbool.h>

typedef enum {
    NUCLEO, TRONCO, BASE, BRACO_E, BRACO_D, PUNHO_E, PUNHO_D, PERNA_E, PERNA_D, NUM_PECAS
} TipoPeca;

/* Lado do filho na árvore (também usado como lado do punho nas ações) */
#define FILHO_ESQ 0
#define FILHO_DIR 1

typedef struct Peca {
    char nome[16];
    TipoPeca tipo;
    int resistencia, resistenciaMax;
    bool ativa;
    struct Peca *esq, *dir;
} Peca;

/* Construção e destruição */
Peca *criarPeca(const char *nome, TipoPeca tipo, int resistencia);
bool  inserirPeca(Peca *raiz, TipoPeca tipoPai, Peca *nova, int lado);
Peca *montarRobo(void);
void  liberarRobo(Peca *raiz);

/* Consultas */
Peca *buscarPeca(Peca *raiz, TipoPeca tipo);
bool  pecaAtiva(Peca *raiz, TipoPeca tipo);
bool  pecaExposta(Peca *raiz, TipoPeca alvo);
int   calcularIntegridade(const Peca *raiz);
int   calcularIntegridadeMaxima(const Peca *raiz);
int   velocidadeDoRobo(Peca *raiz);

/* Dano */
int   aplicarDano(Peca *peca, int dano);
void  desativarSubarvore(Peca *peca);

/* Auxiliares de texto */
const char *nomePeca(TipoPeca tipo);
TipoPeca    punhoDoLado(int lado);
TipoPeca    bracoDoLado(int lado);
const char *preposicaoPeca(TipoPeca tipo);

#endif /* ROBO_H */
