/*
 * config.h
 * Todas as constantes de balanceamento e de interface do Ringue de Aço.
 * Mudar um número aqui muda o jogo inteiro, sem mexer na lógica.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---------- Janela ---------- */
#define LARGURA_JANELA 1280
#define ALTURA_JANELA  720
#define FPS_ALVO       60

/* ---------- Regras gerais ---------- */
#define MAX_RODADAS            10
#define ENERGIA_POR_RODADA     5
#define MAX_ACOES_POR_JOGADOR  3

/* ---------- Resistências iniciais das peças (total 420) ---------- */
#define RES_NUCLEO 60
#define RES_TRONCO 80
#define RES_BASE   70
#define RES_BRACO  40
#define RES_PUNHO  30
#define RES_PERNA  35

/* ---------- Tabela de ações: energia, velocidade base e dano ---------- */
#define ENERGIA_JAB       1
#define VELOCIDADE_JAB    8
#define DANO_JAB          10

#define ENERGIA_DIRETO    2
#define VELOCIDADE_DIRETO 5
#define DANO_DIRETO       20

#define ENERGIA_GANCHO    3
#define VELOCIDADE_GANCHO 3
#define DANO_GANCHO       35

#define ENERGIA_BLOQUEIO    1
#define VELOCIDADE_BLOQUEIO 9
#define DANO_BLOQUEIO       0

#define ENERGIA_ESQUIVA    2
#define VELOCIDADE_ESQUIVA 6
#define DANO_ESQUIVA       0

/* Bônus de velocidade pela posição na lista do jogador (1ª, 2ª e 3ª ação) */
#define BONUS_POSICAO_1 2
#define BONUS_POSICAO_2 1
#define BONUS_POSICAO_3 0

/* O Bloqueio divide o dano dos golpes recebidos (exceto Gancho) por este valor */
#define DIVISOR_BLOQUEIO 2

/* ---------- Interface ---------- */
#define DURACAO_ANIMACAO   1.0f   /* segundos de cada ação na resolução */
#define DURACAO_FILA       1.5f   /* segundos mostrando a fila antes de começar */
#define LIMIAR_VIDA_ALTA   0.60f  /* acima disso a peça fica verde */
#define LIMIAR_VIDA_MEDIA  0.30f  /* acima disso amarela; abaixo vermelha */

#endif /* CONFIG_H */
