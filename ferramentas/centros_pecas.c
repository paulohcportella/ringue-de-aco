/*
 * centros_pecas.c
 * Imprime, para a tela de programação, um ponto bem no meio de cada peça
 * dos dois robôs (o ponto mais "para dentro" da área clicável), no formato
 * de variáveis de shell. Usado por gravar_video.sh para clicar nas peças.
 * Usa a mesma função pecaNoPonto da interface, então o clique é exato.
 */
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "interface.h"
#include "config.h"

static unsigned char mapa[ALTURA_JANELA][LARGURA_JANELA];

/* distância (em pixels, até 60) andando numa direção sem sair da peça */
static int alcance(int x, int y, int dx, int dy, unsigned char t)
{
    int k;
    for (k = 1; k < 60; k++) {
        int nx = x + dx * k, ny = y + dy * k;
        if (nx < 0 || ny < 0 || nx >= LARGURA_JANELA || ny >= ALTURA_JANELA || mapa[ny][nx] != t) {
            return k;
        }
    }
    return 60;
}

static void imprimir(const char *prefixo, Peca *robo, Vector2 pos, bool espelhado)
{
    static const char *nomes[NUM_PECAS] = {
        "NUCLEO", "TRONCO", "BASE", "BRACO_E", "BRACO_D", "PUNHO_E", "PUNHO_D", "PERNA_E", "PERNA_D"
    };
    int x, y, t;
    for (y = 0; y < ALTURA_JANELA; y++) {
        for (x = 0; x < LARGURA_JANELA; x++) {
            mapa[y][x] = (unsigned char)pecaNoPonto(robo, pos, 0.9f, espelhado,
                                                    (Vector2){ (float)x, (float)y }, false);
        }
    }
    for (t = 0; t < NUM_PECAS; t++) {
        int melhor = -1, bx = 0, by = 0;
        for (y = 0; y < ALTURA_JANELA; y++) {
            for (x = 0; x < LARGURA_JANELA; x++) {
                int s, a;
                if (mapa[y][x] != t) {
                    continue;
                }
                s = alcance(x, y, 1, 0, (unsigned char)t);
                a = alcance(x, y, -1, 0, (unsigned char)t); if (a < s) s = a;
                a = alcance(x, y, 0, 1, (unsigned char)t);  if (a < s) s = a;
                a = alcance(x, y, 0, -1, (unsigned char)t); if (a < s) s = a;
                if (s > melhor) {
                    melhor = s; bx = x; by = y;
                }
            }
        }
        printf("%s_%s_X=%d\n%s_%s_Y=%d\n", prefixo, nomes[t], bx, prefixo, nomes[t], by);
    }
}

int main(void)
{
    Batalha b;
    if (!iniciarBatalha(&b)) {
        return 1;
    }
    /* mesmas posições da tela de programação: J1 em (330, 450), J2 espelhado em (950, 450) */
    imprimir("J1", b.robo[0], (Vector2){ 330, 450 }, false);
    imprimir("J2", b.robo[1], (Vector2){ 950, 450 }, true);
    encerrarBatalha(&b);
    return 0;
}
