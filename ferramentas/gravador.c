/*
 * gravador.c
 * Grava o vídeo do jogo sem mexer no código dele. É ligado só no binário de
 * gravação com -Wl,--wrap=EndDrawing: toda chamada a EndDrawing passa por
 * __wrap_EndDrawing, que lê a imagem do quadro direto do OpenGL e manda para
 * o ffmpeg (comando na variável GRAVAR_PIPE), a 30 quadros por segundo de
 * tempo real. Também desenha uma seta onde está o mouse, para o vídeo
 * mostrar os cliques.
 */
#define _POSIX_C_SOURCE 200809L   /* popen/pclose não aparecem em -std=c99 sem isto */
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "rlgl.h"

void __real_EndDrawing(void);

static FILE *saida = NULL;
static double inicio = 0.0;
static long emitidos = 0;

static void desenharSeta(Vector2 m)
{
    Vector2 a = m, b = { m.x, m.y + 22 }, c = { m.x + 15, m.y + 16 };
    DrawTriangle(a, b, c, WHITE);
    DrawTriangleLines(a, b, c, BLACK);
}

void __wrap_EndDrawing(void)
{
    const char *cmd = getenv("GRAVAR_PIPE");
    if (saida == NULL && cmd != NULL) {
        saida = popen(cmd, "w");
        inicio = GetTime();
    }
    if (saida != NULL) {
        long devidos = (long)((GetTime() - inicio) * 30.0) + 1;
        if (emitidos < devidos) {
            int w = GetRenderWidth(), h = GetRenderHeight();
            unsigned char *px;
            desenharSeta(GetMousePosition());
            rlDrawRenderBatchActive();
            px = rlReadScreenPixels(w, h);
            while (emitidos < devidos) {       /* repete o quadro se o jogo atrasou */
                fwrite(px, 1, (size_t)w * h * 4, saida);
                emitidos++;
            }
            free(px);
        }
    }
    __real_EndDrawing();
}
