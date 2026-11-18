/*
 * main.c
 * Ringue de Aço - laço principal e máquina de telas.
 *
 * Fluxo de uma rodada:
 *   Programação J1 -> Troca -> Programação J2 -> Resolução -> Resumo -> Troca -> ...
 * Fim de luta: Núcleo destruído (vitória imediata) ou fim das rodadas (MAX_RODADAS).
 */
#include <stdbool.h>
#include <stddef.h>
#include "raylib.h"
#include "config.h"
#include "batalha.h"
#include "interface.h"

typedef struct {
    Tela tela;
    Batalha batalha;
    bool lutaEmAndamento;     /* há robôs alocados */
    bool resultadoContado;    /* o placar já recebeu o resultado desta luta */
    int jogador;              /* quem está programando (1 ou 2) */
    int rodadaDoResumo;       /* rodada mostrada no resumo */
    int opcaoMenu;
    bool depuracao;           /* sobreposição F1 aberta */
    Placar placar;
    EstadoProgramacao programacao;
    EstadoResolucao resolucao;
} Jogo;

/* novaLuta: libera a luta anterior (se houver) e monta dois robôs novos. */
static bool novaLuta(Jogo *j)
{
    if (j->lutaEmAndamento) {
        encerrarBatalha(&j->batalha);
    }
    j->lutaEmAndamento = iniciarBatalha(&j->batalha);
    j->resultadoContado = false;
    j->jogador = 1;
    j->depuracao = false;
    reiniciarProgramacao(&j->programacao);
    return j->lutaEmAndamento;
}

/* fecharLuta: libera toda a memória da luta atual. */
static void fecharLuta(Jogo *j)
{
    if (j->lutaEmAndamento) {
        encerrarBatalha(&j->batalha);
        j->lutaEmAndamento = false;
    }
    j->depuracao = false;
}

/* contarResultado: soma o resultado no placar da sessão uma única vez. */
static void contarResultado(Jogo *j)
{
    int v = j->batalha.vencedor;
    if (j->resultadoContado || v == 0) {
        return;
    }
    if (v == EMPATE) {
        j->placar.empates++;
    } else {
        j->placar.vitorias[v - 1]++;
    }
    j->resultadoContado = true;
}

/* passoDaMaquina: chama a tela atual e trata a transição que ela pedir. */
static void passoDaMaquina(Jogo *j)
{
    Tela proxima;
    switch (j->tela) {
    case TELA_MENU:
        proxima = menuPrincipal(&j->opcaoMenu);
        if (proxima == TELA_PROGRAMACAO && !novaLuta(j)) {
            proxima = TELA_MENU;   /* sem memória: fica no menu */
        }
        j->tela = proxima;
        break;

    case TELA_COMO_JOGAR:
        j->tela = telaComoJogar();
        break;

    case TELA_PROGRAMACAO:
        proxima = telaProgramacao(&j->batalha, j->jogador, &j->programacao, &j->placar);
        if (proxima == TELA_TROCA) {          /* jogador confirmou a lista */
            if (j->jogador == 1) {
                j->jogador = 2;
                reiniciarProgramacao(&j->programacao);
                j->tela = TELA_TROCA;
            } else {
                prepararResolucao(&j->batalha);   /* une e ordena as listas */
                reiniciarResolucao(&j->resolucao);
                j->tela = TELA_RESOLUCAO;
            }
        }
        break;

    case TELA_TROCA:
        j->tela = telaTroca(j->jogador, j->batalha.rodada);
        break;

    case TELA_RESOLUCAO:
        proxima = telaResolucao(&j->batalha, &j->resolucao);
        if (proxima == TELA_RESUMO || proxima == TELA_FIM) {
            j->rodadaDoResumo = j->batalha.rodada;
            encerrarRodada(&j->batalha);          /* libera listas, vê fim por rodadas */
            contarResultado(j);
            j->tela = proxima;
        }
        break;

    case TELA_RESUMO:
        proxima = telaResumo(&j->batalha, j->rodadaDoResumo);
        if (proxima == TELA_TROCA) {
            j->jogador = 1;
            reiniciarProgramacao(&j->programacao);
        }
        j->tela = proxima;
        break;

    case TELA_FIM:
        proxima = telaFim(&j->batalha, &j->placar);
        if (proxima == TELA_PROGRAMACAO) {        /* R: revanche direto */
            if (!novaLuta(j)) {
                proxima = TELA_MENU;
            }
        } else if (proxima == TELA_MENU) {
            fecharLuta(j);
        }
        j->tela = proxima;
        break;

    default:
        break;
    }
}

/* depuracaoPermitida: F1 funciona em qualquer tela durante a luta. */
static bool depuracaoPermitida(const Jogo *j)
{
    return j->lutaEmAndamento && j->tela != TELA_MENU && j->tela != TELA_COMO_JOGAR
           && j->tela != TELA_SAIR;
}

/* desenharDepuracaoDaTela: escolhe qual lista mostrar sem revelar segredos. */
static void desenharDepuracaoDaTela(Jogo *j)
{
    Batalha *b = &j->batalha;
    switch (j->tela) {
    case TELA_PROGRAMACAO:
        desenharDepuracao(b, b->programacao[j->jogador - 1],
                          j->jogador == 1 ? "Lista do J1 (programação)" : "Lista do J2 (programação)",
                          NULL);
        break;
    case TELA_RESOLUCAO:
        desenharDepuracao(b, b->fila, "Fila de execução (lista unida e ordenada)", b->canceladas);
        break;
    default:
        desenharDepuracao(b, NULL, NULL, NULL);   /* listas secretas ou já liberadas */
        break;
    }
}

int main(void)
{
    static Jogo jogo;   /* static: zerado e fora da pilha */

    jogo.tela = TELA_MENU;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Ringue de Aço");
    SetExitKey(KEY_NULL);   /* ESC não fecha o jogo sem querer */
    SetTargetFPS(FPS_ALVO);

    while (!WindowShouldClose() && jogo.tela != TELA_SAIR) {
        if (IsKeyPressed(KEY_F1) && depuracaoPermitida(&jogo)) {
            jogo.depuracao = !jogo.depuracao;
        }
        if (!depuracaoPermitida(&jogo)) {
            jogo.depuracao = false;
        }
        liberarEntrada(!jogo.depuracao);

        BeginDrawing();
        ClearBackground((Color){ 20, 20, 28, 255 });
        passoDaMaquina(&jogo);
        if (jogo.depuracao && depuracaoPermitida(&jogo)) {
            desenharDepuracaoDaTela(&jogo);
        }
        EndDrawing();
    }

    fecharLuta(&jogo);
    CloseWindow();
    return 0;
}
