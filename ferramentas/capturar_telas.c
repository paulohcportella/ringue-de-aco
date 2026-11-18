/*
 * capturar_telas.c
 * Joga uma luta roteirizada usando as telas reais do jogo (interface.c) e a
 * lógica real (batalha.c) e salva um PNG de cada tela usada nos slides.
 * Usado para gerar as imagens dos slides e, no GitHub Actions, para
 * conferir que o jogo abre no macOS.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "rlgl.h"
#include "config.h"
#include "batalha.h"
#include "interface.h"

static Batalha b;
static Placar placar;


static void salvar(const char *nome)
{
    rlDrawRenderBatchActive();
    TakeScreenshot(nome);
    printf("salvo: %s\n", nome);
}

static void falhar(const char *msg)
{
    fprintf(stderr, "ERRO: %s\n", msg);
    exit(1);
}

/* centroDaPeca: ponto no meio da peça, achado varrendo a tela com pecaNoPonto. */
static Vector2 centroDaPeca(Peca *robo, Vector2 pos, float escala, bool espelhado, TipoPeca alvo)
{
    float sx = 0, sy = 0;
    int n = 0, x, y;
    for (y = 0; y < ALTURA_JANELA; y += 2) {
        for (x = 0; x < LARGURA_JANELA; x += 2) {
            if (pecaNoPonto(robo, pos, escala, espelhado, (Vector2){ (float)x, (float)y }, false) == alvo) {
                sx += x;
                sy += y;
                n++;
            }
        }
    }
    if (n == 0) {
        falhar("peça não encontrada na tela");
    }
    return (Vector2){ sx / n, sy / n };
}

int main(void)
{
    EstadoProgramacao ep;
    EstadoResolucao er;
    Vector2 mira;
    Tela t;
    int opcao = 0, i, quadros;
    bool fotoResolucao = false;

    /* sem MSAA: as máquinas virtuais do GitHub (sem placa de vídeo) não suportam */
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Ringue de Aço");
    SetTargetFPS(FPS_ALVO);
    liberarEntrada(true);

    /* ---------- 0. menu ---------- */
    for (i = 0; i < 20; i++) {
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        menuPrincipal(&opcao);
        if (i == 19) salvar("0-menu.png");
        EndDrawing();
    }

    /* ---------- 1. programação do J1 com o guia no passo 3 ---------- */
    if (!iniciarBatalha(&b)) falhar("iniciarBatalha");
    reiniciarProgramacao(&ep);
    if (!programarAcao(&b, 1, GANCHO, FILHO_DIR, TRONCO, 0, NULL)) falhar("J1 Gancho");
    ep.cursor = 1;
    ep.acao = JAB;
    ep.lado = FILHO_ESQ;
    ep.escolheuAcao = true;
    /* mouse em cima do Braço D do robô azul (posição e escala da tela de programação) */
    mira = centroDaPeca(b.robo[1], (Vector2){ 950, 450 }, 0.9f, true, BRACO_D);
    for (i = 0; i < 40; i++) {
        SetMousePosition((int)mira.x, (int)mira.y);
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        telaProgramacao(&b, 1, &ep, &placar);
        if (i == 39) salvar("1-programacao.png");
        EndDrawing();
    }
    SetMousePosition(5, 5);
    if (!programarAcao(&b, 1, JAB, FILHO_ESQ, BRACO_D, 1, NULL)) falhar("J1 Jab");

    /* J2: Jab e Direto no Braço D do J1 (os dois chegam antes do Gancho) */
    if (!programarAcao(&b, 2, JAB, FILHO_ESQ, BRACO_D, 0, NULL)) falhar("J2 Jab");
    if (!programarAcao(&b, 2, DIRETO, FILHO_DIR, BRACO_D, 1, NULL)) falhar("J2 Direto");

    /* ---------- 2 e 3. resolução: o Direto destrói o braço e o Gancho é cancelado ---------- */
    prepararResolucao(&b);
    reiniciarResolucao(&er);
    for (quadros = 0; quadros < 60 * 30; quadros++) {
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        t = telaResolucao(&b, &er);
        if (!fotoResolucao && er.fase == RES_ANIMANDO && er.evento.acao.tipo == DIRETO
            && er.evento.pecaDestruida && er.tempo >= 0.45f) {
            salvar("2-resolucao.png");
            fotoResolucao = true;
            EndDrawing();

            /* F1: o jogo pausa e desenha as árvores e a fila por cima */
            liberarEntrada(false);
            for (i = 0; i < 4; i++) {
                BeginDrawing();
                ClearBackground((Color){ 20, 20, 28, 255 });
                telaResolucao(&b, &er);
                desenharDepuracao(&b, b.fila, "Fila de execução (lista unida e ordenada)", b.canceladas);
                if (i == 3) salvar("3-depuracao-f1.png");
                EndDrawing();
            }
            liberarEntrada(true);
            continue;
        }
        EndDrawing();
        if (t == TELA_RESUMO || t == TELA_FIM) {
            break;
        }
    }
    if (!fotoResolucao) falhar("o Direto não destruiu o Braço D como esperado");
    if (pecaAtiva(b.robo[0], BRACO_D)) falhar("Braço D do J1 deveria ter caído");
    encerrarRodada(&b);

    /* resumo da rodada 1 */
    for (i = 0; i < 20; i++) {
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        telaResumo(&b, 1);
        if (i == 19) salvar("4-resumo.png");
        EndDrawing();
    }

    /* ---------- rodadas 2 e 3: J1 derruba a Base e depois o Núcleo ---------- */
    for (i = 0; i < 2 && b.vencedor == 0; i++) {
        TipoPeca alvo = pecaExposta(b.robo[1], NUCLEO) ? NUCLEO : BASE;
        programarAcao(&b, 1, GANCHO, FILHO_ESQ, alvo, 0, NULL);
        programarAcao(&b, 1, DIRETO, FILHO_ESQ, alvo, 1, NULL);
        programarAcao(&b, 2, BLOQUEIO, FILHO_ESQ, NUM_PECAS, 0, NULL);
        resolverRodada(&b);
    }
    if (b.vencedor != 1 || !b.vitoriaPorNucleo) falhar("o J1 deveria vencer pelo Núcleo");
    placar.vitorias[0] = 1;

    /* ---------- 5. fim da luta ---------- */
    for (i = 0; i < 90; i++) {
        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        telaFim(&b, &placar);
        if (i == 89) salvar("5-fim.png");
        EndDrawing();
    }

    encerrarBatalha(&b);
    CloseWindow();
    printf("pronto: %d rodadas, vencedor J%d\n", b.rodada, b.vencedor);
    return 0;
}
