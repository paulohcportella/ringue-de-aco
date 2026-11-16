/*
 * acoes.c
 * Lista simplesmente encadeada de ações e Insertion Sort por velocidade.
 * Seja n o número de nós da lista (no máximo 6 na lista unida).
 */
#include <stdlib.h>
#include "acoes.h"
#include "config.h"

/* ---------- Tabela de ações ---------- */

/* ehGolpe: Jab, Direto e Gancho são golpes (usam punho e têm alvo). O(1). */
bool ehGolpe(TipoAcao tipo)
{
    return tipo == JAB || tipo == DIRETO || tipo == GANCHO;
}

/* custoAcao: energia gasta pela ação. O(1). */
int custoAcao(TipoAcao tipo)
{
    switch (tipo) {
    case JAB:      return ENERGIA_JAB;
    case DIRETO:   return ENERGIA_DIRETO;
    case GANCHO:   return ENERGIA_GANCHO;
    case BLOQUEIO: return ENERGIA_BLOQUEIO;
    case ESQUIVA:  return ENERGIA_ESQUIVA;
    default:       return 0;
    }
}

/* velocidadeBase: velocidade da ação antes dos bônus. O(1). */
int velocidadeBase(TipoAcao tipo)
{
    switch (tipo) {
    case JAB:      return VELOCIDADE_JAB;
    case DIRETO:   return VELOCIDADE_DIRETO;
    case GANCHO:   return VELOCIDADE_GANCHO;
    case BLOQUEIO: return VELOCIDADE_BLOQUEIO;
    case ESQUIVA:  return VELOCIDADE_ESQUIVA;
    default:       return 0;
    }
}

/* danoAcao: dano cheio do golpe (0 para Bloqueio e Esquiva). O(1). */
int danoAcao(TipoAcao tipo)
{
    switch (tipo) {
    case JAB:      return DANO_JAB;
    case DIRETO:   return DANO_DIRETO;
    case GANCHO:   return DANO_GANCHO;
    case BLOQUEIO: return DANO_BLOQUEIO;
    case ESQUIVA:  return DANO_ESQUIVA;
    default:       return 0;
    }
}

/* bonusPosicao: 1ª ação +2, 2ª +1, 3ª +0. O(1). */
int bonusPosicao(int posicao)
{
    switch (posicao) {
    case 0:  return BONUS_POSICAO_1;
    case 1:  return BONUS_POSICAO_2;
    default: return BONUS_POSICAO_3;
    }
}

/* nomeAcao: nome legível da ação. O(1). */
const char *nomeAcao(TipoAcao tipo)
{
    switch (tipo) {
    case JAB:      return "Jab";
    case DIRETO:   return "Direto";
    case GANCHO:   return "Gancho";
    case BLOQUEIO: return "Bloqueio";
    case ESQUIVA:  return "Esquiva";
    default:       return "?";
    }
}

/* ---------- Criação e liberação ---------- */

/*
 * criarAcao: aloca um nó de ação com malloc. Para Bloqueio e Esquiva, lado e
 * alvo são ignorados. Devolve NULL se faltar memória. O(1).
 */
Acao *criarAcao(int jogador, TipoAcao tipo, int lado, TipoPeca alvo)
{
    Acao *a = (Acao *)malloc(sizeof(Acao));
    if (a == NULL) {
        return NULL;
    }
    a->jogador = jogador;
    a->tipo = tipo;
    a->lado = ehGolpe(tipo) ? lado : 0;
    a->alvo = ehGolpe(tipo) ? alvo : NUM_PECAS;
    a->posicao = 0;
    a->velocidadeEfetiva = velocidadeBase(tipo);
    a->ordemIntercalacao = 0;
    a->prox = NULL;
    return a;
}

/* liberarLista: libera todos os nós, um a um, do início ao fim. O(n). */
void liberarLista(Acao *lista)
{
    while (lista != NULL) {
        Acao *prox = lista->prox;
        free(lista);
        lista = prox;
    }
}

/* ---------- Consultas ---------- */

/* tamanhoLista: conta os nós. O(n). */
int tamanhoLista(const Acao *lista)
{
    int n = 0;
    while (lista != NULL) {
        n++;
        lista = lista->prox;
    }
    return n;
}

/* acaoNaPosicao: devolve o nó de índice "indice" (0 = primeiro) ou NULL. O(n). */
Acao *acaoNaPosicao(Acao *lista, int indice)
{
    int i = 0;
    while (lista != NULL && i < indice) {
        lista = lista->prox;
        i++;
    }
    return (indice < 0) ? NULL : lista;
}

/* energiaDaLista: soma o custo de energia das ações. O(n). */
int energiaDaLista(const Acao *lista)
{
    int total = 0;
    while (lista != NULL) {
        total += custoAcao(lista->tipo);
        lista = lista->prox;
    }
    return total;
}

/*
 * acaoValidaParaRobo: um golpe exige o punho do lado escolhido ativo; a
 * Esquiva exige a Base ativa; o Bloqueio é sempre válido. O(n) pela busca.
 */
bool acaoValidaParaRobo(const Acao *a, Peca *roboDono)
{
    if (ehGolpe(a->tipo)) {
        return pecaAtiva(roboDono, punhoDoLado(a->lado));
    }
    if (a->tipo == ESQUIVA) {
        return pecaAtiva(roboDono, BASE);
    }
    return true;
}

/* ---------- Edição da lista de um jogador ---------- */

/*
 * renumerarPosicoes: grava 0, 1, 2... no campo posicao, na ordem atual da
 * lista. É o que faz o bônus de posição seguir a ordem montada. O(n).
 */
void renumerarPosicoes(Acao *lista)
{
    int i = 0;
    while (lista != NULL) {
        lista->posicao = i++;
        lista = lista->prox;
    }
}

/*
 * inserirAcao: insere o nó "nova" no índice "posicao" (0 = início,
 * tamanho = fim), reencadeando ponteiros. Depois renumera as posições.
 * Devolve false se o índice for inválido. O(n).
 */
bool inserirAcao(Acao **lista, Acao *nova, int posicao)
{
    Acao **pp = lista;   /* ponteiro para o "prox" que vai apontar para nova */
    int i;
    if (lista == NULL || nova == NULL || posicao < 0) {
        return false;
    }
    for (i = 0; i < posicao; i++) {
        if (*pp == NULL) {
            return false;   /* posição além do fim */
        }
        pp = &(*pp)->prox;
    }
    nova->prox = *pp;
    *pp = nova;
    renumerarPosicoes(*lista);
    return true;
}

/*
 * removerAcao: retira da lista o nó de índice "posicao", libera esse nó e
 * renumera as posições. Devolve false se o índice não existir. O(n).
 */
bool removerAcao(Acao **lista, int posicao)
{
    Acao **pp = lista;
    Acao *alvo;
    int i;
    if (lista == NULL || posicao < 0) {
        return false;
    }
    for (i = 0; i < posicao && *pp != NULL; i++) {
        pp = &(*pp)->prox;
    }
    alvo = *pp;
    if (alvo == NULL) {
        return false;
    }
    *pp = alvo->prox;   /* o anterior passa a apontar para o seguinte */
    free(alvo);
    renumerarPosicoes(*lista);
    return true;
}

/*
 * trocarAcoes: troca de lugar os nós de índices i e j reencadeando ponteiros
 * (os valores dentro dos nós não são copiados). Depois renumera. O(n).
 *
 * Técnica: pi e pj são os ponteiros que apontam para os nós i e j (o "prox"
 * do anterior ou a própria cabeça). Trocar *pi com *pj troca quem aponta para
 * cada nó; trocar os "prox" dos dois nós devolve o resto da lista. Funciona
 * também quando os nós são vizinhos.
 */
bool trocarAcoes(Acao **lista, int i, int j)
{
    Acao **pi, **pj, *tmp;
    int k;
    if (lista == NULL || i < 0 || j < 0) {
        return false;
    }
    if (i == j) {
        return acaoNaPosicao(*lista, i) != NULL;
    }
    if (i > j) {
        k = i; i = j; j = k;
    }
    pi = lista;
    for (k = 0; k < i && *pi != NULL; k++) {
        pi = &(*pi)->prox;
    }
    pj = lista;
    for (k = 0; k < j && *pj != NULL; k++) {
        pj = &(*pj)->prox;
    }
    if (*pi == NULL || *pj == NULL) {
        return false;
    }
    tmp = *pi; *pi = *pj; *pj = tmp;                         /* troca quem aponta */
    tmp = (*pi)->prox; (*pi)->prox = (*pj)->prox; (*pj)->prox = tmp; /* troca os prox */
    renumerarPosicoes(*lista);
    return true;
}

/* ---------- Resolução ---------- */

/*
 * unirListas: intercala as duas listas movendo os nós (sem copiar):
 * A1, B1, A2, B2, A3, B3, onde A é a lista de quem começa (1 ou 2).
 * Grava em ordemIntercalacao a posição de cada nó na lista unida, que é o
 * critério de desempate. As listas originais deixam de existir. O(n).
 */
Acao *unirListas(Acao *listaJ1, Acao *listaJ2, int quemComeca)
{
    Acao *primeira = (quemComeca == 2) ? listaJ2 : listaJ1;
    Acao *segunda  = (quemComeca == 2) ? listaJ1 : listaJ2;
    Acao *cabeca = NULL;
    Acao **fim = &cabeca;   /* onde o próximo nó será pendurado */
    int ordem = 0;
    Acao *no;

    while (primeira != NULL || segunda != NULL) {
        if (primeira != NULL) {
            no = primeira;
            primeira = primeira->prox;
            no->prox = NULL;
            no->ordemIntercalacao = ordem++;
            *fim = no;
            fim = &no->prox;
        }
        if (segunda != NULL) {
            no = segunda;
            segunda = segunda->prox;
            no->prox = NULL;
            no->ordemIntercalacao = ordem++;
            *fim = no;
            fim = &no->prox;
        }
    }
    return cabeca;
}

/* vemAntes: critério da ordenação. Maior velocidade primeiro; empate, menor
 * ordemIntercalacao primeiro. O(1). */
static bool vemAntes(const Acao *a, const Acao *b)
{
    if (a->velocidadeEfetiva != b->velocidadeEfetiva) {
        return a->velocidadeEfetiva > b->velocidadeEfetiva;
    }
    return a->ordemIntercalacao < b->ordemIntercalacao;
}

/*
 * ordenarPorVelocidade: Insertion Sort na lista encadeada, em ordem
 * decrescente de velocidadeEfetiva (empate: ordemIntercalacao crescente).
 *
 * Tira os nós da lista original um a um e os encaixa numa lista "ordenada",
 * só reencadeando ponteiros (nenhum valor é trocado entre nós). Se o nó vem
 * depois do último da lista ordenada, ele é pendurado no fim em O(1);
 * senão, percorre a lista ordenada até achar o lugar.
 *
 * Complexidade: O(n²) no pior caso (lista em ordem inversa),
 * O(n) no melhor caso (lista já ordenada). Memória extra O(1).
 * Estável: nós de mesma velocidade mantêm a ordem de intercalação.
 */
void ordenarPorVelocidade(Acao **lista)
{
    Acao *ordenada = NULL;   /* cabeça da parte já ordenada */
    Acao *ultimo = NULL;     /* último nó da parte ordenada */
    Acao *atual, *proximo;
    Acao **pp;

    if (lista == NULL) {
        return;
    }
    atual = *lista;
    while (atual != NULL) {
        proximo = atual->prox;
        if (ordenada == NULL || !vemAntes(atual, ultimo)) {
            /* vai para o fim da parte ordenada */
            atual->prox = NULL;
            if (ordenada == NULL) {
                ordenada = atual;
            } else {
                ultimo->prox = atual;
            }
            ultimo = atual;
        } else {
            /* procura o primeiro nó que deve vir depois de "atual" */
            pp = &ordenada;
            while (vemAntes(*pp, atual)) {
                pp = &(*pp)->prox;
            }
            atual->prox = *pp;
            *pp = atual;
        }
        atual = proximo;
    }
    *lista = ordenada;
}

/*
 * removerAcoesInvalidas: retira da lista os golpes cujo punho está inativo e
 * as esquivas com Base inativa. Os nós retirados não são liberados aqui: são
 * movidos para o fim da lista "canceladas", para a interface mostrá-los
 * riscados (quem chama libera depois com liberarLista).
 * Devolve quantas ações foram canceladas. O(n) nós, cada um com busca O(9).
 */
int removerAcoesInvalidas(Acao **lista, Peca *roboJ1, Peca *roboJ2, Acao **canceladas)
{
    Acao **pp = lista;
    Acao **fimCanceladas = canceladas;
    Acao *no;
    int removidas = 0;

    if (lista == NULL || canceladas == NULL) {
        return 0;
    }
    while (*fimCanceladas != NULL) {
        fimCanceladas = &(*fimCanceladas)->prox;
    }
    while (*pp != NULL) {
        no = *pp;
        if (!acaoValidaParaRobo(no, (no->jogador == 1) ? roboJ1 : roboJ2)) {
            *pp = no->prox;          /* desliga o nó da lista */
            no->prox = NULL;
            *fimCanceladas = no;     /* pendura no fim das canceladas */
            fimCanceladas = &no->prox;
            removidas++;
        } else {
            pp = &no->prox;
        }
    }
    return removidas;
}

/*
 * retirarPrimeira: desliga e devolve o primeiro nó (ou NULL se vazia).
 * Quem chama executa a ação e libera o nó. O(1).
 */
Acao *retirarPrimeira(Acao **lista)
{
    Acao *primeiro;
    if (lista == NULL || *lista == NULL) {
        return NULL;
    }
    primeiro = *lista;
    *lista = primeiro->prox;
    primeiro->prox = NULL;
    return primeiro;
}
