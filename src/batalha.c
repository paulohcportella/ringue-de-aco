/*
 * batalha.c
 * Regras do duelo: validação, resolução da rodada e vencedor.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batalha.h"
#include "config.h"

/* registrar: guarda uma linha de texto no registro da rodada (para o resumo). */
static void registrar(Batalha *b, const char *formato, ...)
{
    va_list args;
    if (b->numRegistros >= MAX_LOG) {
        return;
    }
    va_start(args, formato);
    vsnprintf(b->registro[b->numRegistros], TAM_TEXTO, formato, args);
    va_end(args);
    b->numRegistros++;
}

/* ---------- Início e fim de uma luta ---------- */

/*
 * iniciarBatalha: monta os dois robôs e zera o estado da luta.
 * Devolve false se faltar memória. O(n).
 */
bool iniciarBatalha(Batalha *b)
{
    memset(b, 0, sizeof(*b));
    b->robo[0] = montarRobo();
    b->robo[1] = montarRobo();
    b->rodada = 1;
    if (b->robo[0] == NULL || b->robo[1] == NULL) {
        encerrarBatalha(b);
        return false;
    }
    return true;
}

/* encerrarBatalha: libera os robôs (pós-ordem) e todas as listas. O(n). */
void encerrarBatalha(Batalha *b)
{
    int j;
    for (j = 0; j < 2; j++) {
        liberarRobo(b->robo[j]);
        b->robo[j] = NULL;
        liberarLista(b->programacao[j]);
        b->programacao[j] = NULL;
    }
    liberarLista(b->fila);
    b->fila = NULL;
    liberarLista(b->canceladas);
    b->canceladas = NULL;
}

/* ---------- Programação ---------- */

/* energiaRestante: energia da rodada menos o custo das ações já programadas. O(n). */
int energiaRestante(const Batalha *b, int jogador)
{
    return ENERGIA_POR_RODADA - energiaDaLista(b->programacao[jogador - 1]);
}

/*
 * velocidadePrevista: velocidade que a ação teria na posição dada:
 * base + bônus de posição + velocidade do robô. O(n).
 */
int velocidadePrevista(Batalha *b, int jogador, TipoAcao tipo, int posicao)
{
    return velocidadeBase(tipo) + bonusPosicao(posicao) + velocidadeDoRobo(b->robo[jogador - 1]);
}

/*
 * validarAcao: confere se o jogador pode programar a ação agora.
 * Regras: no máximo 3 ações, energia suficiente, golpe com punho do lado
 * ativo e alvo exposto no rival, Esquiva com Base ativa.
 * Se não puder, *motivo recebe o texto do erro. O(n).
 */
bool validarAcao(Batalha *b, int jogador, TipoAcao tipo, int lado, TipoPeca alvo,
                 const char **motivo)
{
    Peca *meuRobo = b->robo[jogador - 1];
    Peca *rival = b->robo[2 - jogador];
    const char *erro = NULL;

    if ((int)tipo < 0 || tipo >= NUM_ACOES) {
        erro = "Ação inválida";
    } else if (tamanhoLista(b->programacao[jogador - 1]) >= MAX_ACOES_POR_JOGADOR) {
        erro = "Lista cheia: no máximo 3 ações";
    } else if (custoAcao(tipo) > energiaRestante(b, jogador)) {
        erro = "Energia insuficiente";
    } else if (ehGolpe(tipo) && !pecaAtiva(meuRobo, punhoDoLado(lado))) {
        erro = (lado == FILHO_ESQ) ? "Punho esquerdo inativo" : "Punho direito inativo";
    } else if (ehGolpe(tipo) && ((int)alvo < 0 || alvo >= NUM_PECAS || !pecaExposta(rival, alvo))) {
        erro = "Essa peça do rival não pode ser atingida";
    } else if (tipo == ESQUIVA && !pecaAtiva(meuRobo, BASE)) {
        erro = "Base destruída: sem esquiva";
    }
    if (motivo != NULL) {
        *motivo = erro;
    }
    return erro == NULL;
}

/*
 * programarAcao: valida, cria o nó e o insere na lista do jogador no índice
 * "posicao" (limitado ao tamanho atual). O(n).
 */
bool programarAcao(Batalha *b, int jogador, TipoAcao tipo, int lado, TipoPeca alvo,
                   int posicao, const char **motivo)
{
    Acao *nova;
    int tam;
    if (!validarAcao(b, jogador, tipo, lado, alvo, motivo)) {
        return false;
    }
    nova = criarAcao(jogador, tipo, lado, alvo);
    if (nova == NULL) {
        if (motivo != NULL) {
            *motivo = "Sem memória";
        }
        return false;
    }
    tam = tamanhoLista(b->programacao[jogador - 1]);
    if (posicao < 0) {
        posicao = 0;
    }
    if (posicao > tam) {
        posicao = tam;
    }
    inserirAcao(&b->programacao[jogador - 1], nova, posicao);
    return true;
}

/* ---------- Resolução ---------- */

/*
 * atualizarVelocidades: recalcula velocidadeEfetiva de cada ação da fila:
 * base + bônus da posição original + velocidade atual do robô dono. O(n).
 */
void atualizarVelocidades(Batalha *b)
{
    Acao *a;
    for (a = b->fila; a != NULL; a = a->prox) {
        a->velocidadeEfetiva = velocidadeBase(a->tipo) + bonusPosicao(a->posicao)
                               + velocidadeDoRobo(b->robo[a->jogador - 1]);
    }
}

/* motivoCancelamento: explica por que a ação ficou inválida. O(1). */
static const char *motivoCancelamento(const Acao *a, Peca *roboDono)
{
    if (a->tipo == ESQUIVA) {
        return "Base destruída";
    }
    if (!pecaAtiva(roboDono, TRONCO)) {
        return "Tronco destruído";
    }
    if (!pecaAtiva(roboDono, bracoDoLado(a->lado))) {
        return (a->lado == FILHO_ESQ) ? "Braço E destruído" : "Braço D destruído";
    }
    return (a->lado == FILHO_ESQ) ? "Punho E destruído" : "Punho D destruído";
}

/*
 * cancelarInvalidas: chama removerAcoesInvalidas e escreve no registro uma
 * linha para cada ação cancelada. Devolve quantas foram canceladas. O(n).
 */
static int cancelarInvalidas(Batalha *b, Evento *ev)
{
    int antes = tamanhoLista(b->canceladas);
    int n = removerAcoesInvalidas(&b->fila, b->robo[0], b->robo[1], &b->canceladas);
    Acao *a = acaoNaPosicao(b->canceladas, antes);
    if (ev != NULL) {
        ev->canceladasNoPasso = n;
        ev->textoCancelamento[0] = '\0';
    }
    for (; a != NULL; a = a->prox) {
        const char *motivo = motivoCancelamento(a, b->robo[a->jogador - 1]);
        registrar(b, "J%d: %s, %s cancelado", a->jogador, motivo, nomeAcao(a->tipo));
        if (ev != NULL) {
            snprintf(ev->textoCancelamento, TAM_TEXTO, "J%d: %s, %s cancelado",
                     a->jogador, motivo, nomeAcao(a->tipo));
        }
    }
    return n;
}

/*
 * prepararResolucao: une as listas dos dois jogadores (J1 começa nas rodadas
 * ímpares, J2 nas pares), calcula as velocidades, cancela o que já for
 * inválido e ordena com o Insertion Sort. O(n²) pela ordenação.
 */
void prepararResolucao(Batalha *b)
{
    int quemComeca = (b->rodada % 2 == 1) ? 1 : 2;
    b->fila = unirListas(b->programacao[0], b->programacao[1], quemComeca);
    b->programacao[0] = NULL;
    b->programacao[1] = NULL;
    b->bloqueioAtivo[0] = b->bloqueioAtivo[1] = false;
    b->esquivasPendentes[0] = b->esquivasPendentes[1] = 0;
    b->numRegistros = 0;
    atualizarVelocidades(b);
    cancelarInvalidas(b, NULL);
    ordenarPorVelocidade(&b->fila);
}

/*
 * executarGolpe: aplica as regras de um golpe do jogador p no rival q.
 * Ordem: alvo não exposto => golpe no vazio; Esquiva pendente => anula;
 * Bloqueio ativo => metade do dano (exceto Gancho).
 */
static void executarGolpe(Batalha *b, const Acao *a, Evento *ev)
{
    int p = a->jogador - 1;
    int q = 1 - p;
    int dano;
    Peca *alvo;
    char extra[64] = "";

    if (!pecaExposta(b->robo[q], a->alvo)) {
        ev->noVazio = true;
        snprintf(ev->texto, TAM_TEXTO, "J%d: %s %s %s de J%d... golpe no vazio",
                 a->jogador, nomeAcao(a->tipo), preposicaoPeca(a->alvo), nomePeca(a->alvo), q + 1);
        return;
    }
    if (b->esquivasPendentes[q] > 0) {
        b->esquivasPendentes[q]--;
        ev->esquivado = true;
        snprintf(ev->texto, TAM_TEXTO, "J%d esquivou do %s de J%d!",
                 q + 1, nomeAcao(a->tipo), a->jogador);
        return;
    }
    dano = danoAcao(a->tipo);
    if (b->bloqueioAtivo[q] && a->tipo != GANCHO) {
        dano /= DIVISOR_BLOQUEIO;
        ev->bloqueado = true;
    }
    alvo = buscarPeca(b->robo[q], a->alvo);
    ev->danoCausado = aplicarDano(alvo, dano);
    ev->pecaDestruida = !alvo->ativa;
    if (ev->pecaDestruida) {
        snprintf(extra, sizeof(extra), " (%s %s!)", nomePeca(a->alvo),
                 (a->alvo == BASE || a->alvo == PERNA_E || a->alvo == PERNA_D) ? "destruída" : "destruído");
    }
    snprintf(ev->texto, TAM_TEXTO, "J%d: %s %s %s de J%d, -%d%s%s",
             a->jogador, nomeAcao(a->tipo), preposicaoPeca(a->alvo), nomePeca(a->alvo), q + 1, ev->danoCausado,
             ev->bloqueado ? " (bloqueado)" : "", extra);
}

/*
 * executarProximaAcao: um passo da resolução.
 * 1. retira o primeiro nó da fila e executa a ação;
 * 2. libera o nó;
 * 3. recalcula as velocidades (as pernas podem ter caído);
 * 4. remove as ações que ficaram inválidas;
 * 5. reordena com o Insertion Sort;
 * 6. verifica se algum núcleo caiu.
 * Devolve false se não havia o que executar. O(n²) pela ordenação.
 */
bool executarProximaAcao(Batalha *b, Evento *ev)
{
    Acao *atual;
    Evento local;
    int p;

    if (ev == NULL) {
        ev = &local;
    }
    memset(ev, 0, sizeof(*ev));
    if (b->vencedor != 0) {
        return false;
    }
    atual = retirarPrimeira(&b->fila);
    if (atual == NULL) {
        return false;
    }
    ev->executou = true;
    ev->acao = *atual;
    ev->acao.prox = NULL;
    p = atual->jogador - 1;

    if (!acaoValidaParaRobo(atual, b->robo[p])) {
        /* segurança: não deveria acontecer, pois as inválidas já saem antes */
        atual->prox = b->canceladas;
        b->canceladas = atual;
        snprintf(ev->texto, TAM_TEXTO, "J%d: %s cancelado", atual->jogador, nomeAcao(atual->tipo));
        registrar(b, "%s", ev->texto);
        return true;
    }

    switch (atual->tipo) {
    case BLOQUEIO:
        b->bloqueioAtivo[p] = true;
        snprintf(ev->texto, TAM_TEXTO, "J%d levanta a guarda (Bloqueio)", atual->jogador);
        break;
    case ESQUIVA:
        b->esquivasPendentes[p]++;
        snprintf(ev->texto, TAM_TEXTO, "J%d prepara a Esquiva", atual->jogador);
        break;
    default:
        executarGolpe(b, atual, ev);
        break;
    }
    registrar(b, "%s", ev->texto);
    free(atual);

    atualizarVelocidades(b);
    cancelarInvalidas(b, ev);
    ordenarPorVelocidade(&b->fila);
    verificarVencedor(b, false);
    return true;
}

/*
 * resolverRodada: resolve a rodada inteira de uma vez (usado nos testes; a
 * interface chama os mesmos passos um a um para poder animar):
 * prepara a fila e executa ação por ação até a fila esvaziar ou um núcleo
 * cair. Por fim encerra a rodada.
 */
void resolverRodada(Batalha *b)
{
    prepararResolucao(b);
    while (executarProximaAcao(b, NULL)) {
        /* cada chamada executa, remove e libera um nó */
    }
    encerrarRodada(b);
}

/*
 * encerrarRodada: libera o que sobrou das listas, verifica o fim por
 * rodadas e avança o contador de rodada se a luta continua. O(n).
 */
void encerrarRodada(Batalha *b)
{
    liberarLista(b->fila);
    b->fila = NULL;
    liberarLista(b->canceladas);
    b->canceladas = NULL;
    liberarLista(b->programacao[0]);
    liberarLista(b->programacao[1]);
    b->programacao[0] = b->programacao[1] = NULL;
    if (verificarVencedor(b, true) == 0) {
        b->rodada++;
    }
}

/* ---------- Fim de luta ---------- */

/*
 * verificarVencedor: núcleo destruído dá vitória imediata a quem destruiu.
 * Se "fimDaRodada" e a rodada atual é a última, vence a maior integridade
 * (igual = EMPATE). Grava e devolve b->vencedor (0 = luta continua). O(n).
 */
int verificarVencedor(Batalha *b, bool fimDaRodada)
{
    int i1, i2;
    if (b->vencedor != 0) {
        return b->vencedor;
    }
    if (!pecaAtiva(b->robo[1], NUCLEO)) {
        b->vencedor = 1;
        b->vitoriaPorNucleo = true;
    } else if (!pecaAtiva(b->robo[0], NUCLEO)) {
        b->vencedor = 2;
        b->vitoriaPorNucleo = true;
    } else if (fimDaRodada && b->rodada >= MAX_RODADAS) {
        i1 = calcularIntegridade(b->robo[0]);
        i2 = calcularIntegridade(b->robo[1]);
        b->vencedor = (i1 > i2) ? 1 : (i2 > i1) ? 2 : EMPATE;
        b->vitoriaPorNucleo = false;
    }
    return b->vencedor;
}
