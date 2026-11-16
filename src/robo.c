/*
 * robo.c
 * Implementação da árvore binária de peças do robô.
 * Seja n o número de peças (n = 9 no robô montado).
 */
#include <stdlib.h>
#include <string.h>
#include "robo.h"
#include "config.h"

/*
 * criarPeca: aloca uma peça nova com malloc, ativa, com resistência cheia
 * e sem filhos. Devolve NULL se faltar memória. O(1).
 */
Peca *criarPeca(const char *nome, TipoPeca tipo, int resistencia)
{
    Peca *p = (Peca *)malloc(sizeof(Peca));
    if (p == NULL) {
        return NULL;
    }
    strncpy(p->nome, nome, sizeof(p->nome) - 1);
    p->nome[sizeof(p->nome) - 1] = '\0';
    p->tipo = tipo;
    p->resistencia = resistencia;
    p->resistenciaMax = resistencia;
    p->ativa = true;
    p->esq = NULL;
    p->dir = NULL;
    return p;
}

/*
 * inserirPeca: pendura a peça "nova" como filho (esquerdo ou direito,
 * conforme "lado") da peça do tipo "tipoPai". Usa buscarPeca para achar o pai.
 * Falha (devolve false) se o pai não existir ou se aquele lado já estiver
 * ocupado. O(n), por causa da busca.
 */
bool inserirPeca(Peca *raiz, TipoPeca tipoPai, Peca *nova, int lado)
{
    Peca *pai;
    if (raiz == NULL || nova == NULL) {
        return false;
    }
    pai = buscarPeca(raiz, tipoPai);
    if (pai == NULL) {
        return false;
    }
    if (lado == FILHO_ESQ) {
        if (pai->esq != NULL) {
            return false;
        }
        pai->esq = nova;
    } else {
        if (pai->dir != NULL) {
            return false;
        }
        pai->dir = nova;
    }
    return true;
}

/* Auxiliar de montarRobo: cria a peça e a insere; se falhar, libera. */
static bool anexar(Peca *raiz, TipoPeca tipoPai, int lado,
                   const char *nome, TipoPeca tipo, int resistencia)
{
    Peca *nova = criarPeca(nome, tipo, resistencia);
    if (!inserirPeca(raiz, tipoPai, nova, lado)) {
        free(nova);
        return false;
    }
    return true;
}

/*
 * montarRobo: constrói a árvore de 9 peças no formato fixo do jogo, com as
 * resistências de config.h. Devolve a raiz (o Núcleo) ou NULL se faltar
 * memória. O(n²) no total (n inserções, cada uma com busca O(n)); com n = 9
 * isso é desprezível.
 */
Peca *montarRobo(void)
{
    bool ok = true;
    Peca *raiz = criarPeca("Núcleo", NUCLEO, RES_NUCLEO);
    if (raiz == NULL) {
        return NULL;
    }
    ok = ok && anexar(raiz, NUCLEO,  FILHO_ESQ, "Tronco",  TRONCO,  RES_TRONCO);
    ok = ok && anexar(raiz, NUCLEO,  FILHO_DIR, "Base",    BASE,    RES_BASE);
    ok = ok && anexar(raiz, TRONCO,  FILHO_ESQ, "Braço E", BRACO_E, RES_BRACO);
    ok = ok && anexar(raiz, TRONCO,  FILHO_DIR, "Braço D", BRACO_D, RES_BRACO);
    ok = ok && anexar(raiz, BASE,    FILHO_ESQ, "Perna E", PERNA_E, RES_PERNA);
    ok = ok && anexar(raiz, BASE,    FILHO_DIR, "Perna D", PERNA_D, RES_PERNA);
    ok = ok && anexar(raiz, BRACO_E, FILHO_ESQ, "Punho E", PUNHO_E, RES_PUNHO);
    ok = ok && anexar(raiz, BRACO_D, FILHO_ESQ, "Punho D", PUNHO_D, RES_PUNHO);
    if (!ok) {
        liberarRobo(raiz);
        return NULL;
    }
    return raiz;
}

/*
 * buscarPeca: busca recursiva em pré-ordem (visita a raiz, depois a
 * subárvore esquerda, depois a direita). Devolve o ponteiro para a peça do
 * tipo pedido ou NULL. O(n) no pior caso, pois pode visitar todas as peças.
 */
Peca *buscarPeca(Peca *raiz, TipoPeca tipo)
{
    Peca *achada;
    if (raiz == NULL) {
        return NULL;
    }
    if (raiz->tipo == tipo) {
        return raiz;
    }
    achada = buscarPeca(raiz->esq, tipo);
    if (achada != NULL) {
        return achada;
    }
    return buscarPeca(raiz->dir, tipo);
}

/* pecaAtiva: true se a peça existe e está ativa. O(n) por causa da busca. */
bool pecaAtiva(Peca *raiz, TipoPeca tipo)
{
    Peca *p = buscarPeca(raiz, tipo);
    return p != NULL && p->ativa;
}

/*
 * desativarSubarvore: marca a peça e todos os seus descendentes como
 * inativos (percurso em pré-ordem). Ex.: Tronco desliga braços e punhos.
 * O(k), onde k é o número de nós da subárvore.
 */
void desativarSubarvore(Peca *peca)
{
    if (peca == NULL) {
        return;
    }
    peca->ativa = false;
    desativarSubarvore(peca->esq);
    desativarSubarvore(peca->dir);
}

/*
 * aplicarDano: reduz a resistência da peça (mínimo 0). Se zerar, chama
 * desativarSubarvore. O dano que passar da resistência é perdido.
 * Devolve o dano realmente aplicado. O(1) + O(k) se a peça cair.
 */
int aplicarDano(Peca *peca, int dano)
{
    int aplicado;
    if (peca == NULL || !peca->ativa || dano <= 0) {
        return 0;
    }
    aplicado = (dano < peca->resistencia) ? dano : peca->resistencia;
    peca->resistencia -= aplicado;
    if (peca->resistencia == 0) {
        desativarSubarvore(peca);
    }
    return aplicado;
}

/*
 * pecaExposta: diz se a peça "alvo" pode ser atingida.
 * - Núcleo: só se estiver ativo e o Tronco ou a Base estiverem inativos.
 * - Demais peças: se estiverem ativas.
 * O(n) por causa das buscas.
 */
bool pecaExposta(Peca *raiz, TipoPeca alvo)
{
    if (!pecaAtiva(raiz, alvo)) {
        return false;
    }
    if (alvo == NUCLEO) {
        return !pecaAtiva(raiz, TRONCO) || !pecaAtiva(raiz, BASE);
    }
    return true;
}

/*
 * calcularIntegridade: percorre a árvore (pré-ordem) somando a resistência
 * das peças ativas. O(n).
 */
int calcularIntegridade(const Peca *raiz)
{
    int soma;
    if (raiz == NULL) {
        return 0;
    }
    soma = raiz->ativa ? raiz->resistencia : 0;
    return soma + calcularIntegridade(raiz->esq) + calcularIntegridade(raiz->dir);
}

/* calcularIntegridadeMaxima: soma das resistências máximas (420). O(n). */
int calcularIntegridadeMaxima(const Peca *raiz)
{
    if (raiz == NULL) {
        return 0;
    }
    return raiz->resistenciaMax + calcularIntegridadeMaxima(raiz->esq)
           + calcularIntegridadeMaxima(raiz->dir);
}

/*
 * velocidadeDoRobo: +1 por perna ativa (0 a 2). Se a Base estiver inativa,
 * as pernas também estão (cascata) e o resultado é 0. O(n) pelas buscas.
 */
int velocidadeDoRobo(Peca *raiz)
{
    int v = 0;
    if (!pecaAtiva(raiz, BASE)) {
        return 0;
    }
    if (pecaAtiva(raiz, PERNA_E)) {
        v++;
    }
    if (pecaAtiva(raiz, PERNA_D)) {
        v++;
    }
    return v;
}

/*
 * liberarRobo: libera a árvore em pós-ordem (filhos antes do pai), para
 * nunca acessar um nó já liberado. O(n).
 */
void liberarRobo(Peca *raiz)
{
    if (raiz == NULL) {
        return;
    }
    liberarRobo(raiz->esq);
    liberarRobo(raiz->dir);
    free(raiz);
}

/* nomePeca: nome legível de cada tipo de peça. O(1). */
const char *nomePeca(TipoPeca tipo)
{
    switch (tipo) {
    case NUCLEO:  return "Núcleo";
    case TRONCO:  return "Tronco";
    case BASE:    return "Base";
    case BRACO_E: return "Braço E";
    case BRACO_D: return "Braço D";
    case PUNHO_E: return "Punho E";
    case PUNHO_D: return "Punho D";
    case PERNA_E: return "Perna E";
    case PERNA_D: return "Perna D";
    default:      return "?";
    }
}

/* punhoDoLado / bracoDoLado: converte o lado da ação (0 E, 1 D) em peça. O(1). */
TipoPeca punhoDoLado(int lado)
{
    return (lado == FILHO_ESQ) ? PUNHO_E : PUNHO_D;
}

TipoPeca bracoDoLado(int lado)
{
    return (lado == FILHO_ESQ) ? BRACO_E : BRACO_D;
}

/* preposicaoPeca: "na" para peças femininas (Base, Pernas), "no" para as demais. O(1). */
const char *preposicaoPeca(TipoPeca tipo)
{
    return (tipo == BASE || tipo == PERNA_E || tipo == PERNA_D) ? "na" : "no";
}
