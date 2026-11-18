/*
 * test_logica.c
 * Testes com assert da árvore, da lista, do Insertion Sort e da resolução.
 * Roda no terminal, sem raylib:  make test
 *
 * Quando compilado com -DCONTAR_MEMORIA e -Wl,--wrap=malloc,--wrap=free
 * (o Makefile faz isso no Linux e no Windows), cada malloc/free é contado e
 * cada teste confere que não sobrou memória alocada.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/robo.h"
#include "../src/acoes.h"
#include "../src/batalha.h"
#include "../src/config.h"

/* Integridade de um robô inteiro, calculada a partir do config.h */
#define TOTAL_RES (RES_NUCLEO + RES_TRONCO + RES_BASE + 2 * RES_BRACO + 2 * RES_PUNHO + 2 * RES_PERNA)

/* ---------- Contador de memória ---------- */
#ifdef CONTAR_MEMORIA
void *__real_malloc(size_t n);
void __real_free(void *p);
static long alocacoesVivas = 0;

void *__wrap_malloc(size_t n)
{
    void *p = __real_malloc(n);
    if (p != NULL) {
        alocacoesVivas++;
    }
    return p;
}

void __wrap_free(void *p)
{
    if (p != NULL) {
        alocacoesVivas--;
    }
    __real_free(p);
}
#define MEMORIA_ATUAL() (alocacoesVivas)
#else
#define MEMORIA_ATUAL() (0L)
#endif

static int testesRodados = 0;

#define TESTE(nome) static void nome(void)
#define RODAR(nome) do {                                              \
        long antes = MEMORIA_ATUAL();                                 \
        nome();                                                       \
        assert(MEMORIA_ATUAL() == antes && "vazamento de memória");   \
        testesRodados++;                                              \
        printf("  ok  %s\n", #nome);                                  \
    } while (0)

/* Monta uma lista a partir de vários nós já criados (auxiliar dos testes). */
static Acao *listaDe(Acao *a, Acao *b, Acao *c)
{
    Acao *lista = NULL;
    if (a) inserirAcao(&lista, a, 0);
    if (b) inserirAcao(&lista, b, 1);
    if (c) inserirAcao(&lista, c, 2);
    return lista;
}

/* ================= Árvore (robô) ================= */

TESTE(teste_montar_e_buscar)
{
    Peca *r = montarRobo();
    int t;
    assert(r != NULL);
    assert(r->tipo == NUCLEO);
    assert(r->esq->tipo == TRONCO && r->dir->tipo == BASE);
    assert(r->esq->esq->tipo == BRACO_E && r->esq->dir->tipo == BRACO_D);
    assert(r->dir->esq->tipo == PERNA_E && r->dir->dir->tipo == PERNA_D);
    assert(r->esq->esq->esq->tipo == PUNHO_E && r->esq->esq->dir == NULL);
    assert(r->esq->dir->esq->tipo == PUNHO_D && r->esq->dir->dir == NULL);
    for (t = 0; t < NUM_PECAS; t++) {
        Peca *p = buscarPeca(r, (TipoPeca)t);
        assert(p != NULL && p->tipo == (TipoPeca)t && p->ativa);
    }
    assert(calcularIntegridade(r) == TOTAL_RES);
    assert(calcularIntegridadeMaxima(r) == TOTAL_RES);
    assert(velocidadeDoRobo(r) == 2);
    /* inserir num lugar ocupado falha */
    {
        Peca *extra = criarPeca("Extra", TRONCO, 10);
        assert(!inserirPeca(r, NUCLEO, extra, FILHO_ESQ));
        free(extra);
    }
    liberarRobo(r);
}

TESTE(teste_dano_minimo_zero_e_excedente_perdido)
{
    Peca *r = montarRobo();
    Peca *punho = buscarPeca(r, PUNHO_E);
    assert(aplicarDano(punho, 10) == 10);
    assert(punho->resistencia == RES_PUNHO - 10 && punho->ativa);
    assert(aplicarDano(punho, 50) == RES_PUNHO - 10);   /* só o que restava */
    assert(punho->resistencia == 0 && !punho->ativa);
    assert(aplicarDano(punho, 10) == 0);    /* peça inativa não recebe dano */
    assert(calcularIntegridade(r) == TOTAL_RES - RES_PUNHO);
    liberarRobo(r);
}

TESTE(teste_cascata_tronco)
{
    Peca *r = montarRobo();
    aplicarDano(buscarPeca(r, TRONCO), RES_TRONCO);
    assert(!pecaAtiva(r, TRONCO));
    assert(!pecaAtiva(r, BRACO_E) && !pecaAtiva(r, BRACO_D));
    assert(!pecaAtiva(r, PUNHO_E) && !pecaAtiva(r, PUNHO_D));
    assert(pecaAtiva(r, NUCLEO) && pecaAtiva(r, BASE));
    assert(pecaAtiva(r, PERNA_E) && pecaAtiva(r, PERNA_D));
    /* total - tronco - 2 braços - 2 punhos */
    assert(calcularIntegridade(r) == TOTAL_RES - RES_TRONCO - 2 * RES_BRACO - 2 * RES_PUNHO);
    liberarRobo(r);
}

TESTE(teste_cascata_braco_e_base)
{
    Peca *r = montarRobo();
    aplicarDano(buscarPeca(r, BRACO_D), RES_BRACO);
    assert(!pecaAtiva(r, BRACO_D) && !pecaAtiva(r, PUNHO_D));
    assert(pecaAtiva(r, BRACO_E) && pecaAtiva(r, PUNHO_E));

    aplicarDano(buscarPeca(r, PERNA_E), RES_PERNA);
    assert(velocidadeDoRobo(r) == 1);
    aplicarDano(buscarPeca(r, BASE), RES_BASE);
    assert(!pecaAtiva(r, PERNA_D));
    assert(velocidadeDoRobo(r) == 0);
    liberarRobo(r);
}

TESTE(teste_nucleo_exposto)
{
    Peca *r = montarRobo();
    assert(!pecaExposta(r, NUCLEO));
    assert(pecaExposta(r, TRONCO) && pecaExposta(r, PUNHO_E));
    aplicarDano(buscarPeca(r, BASE), RES_BASE);
    assert(pecaExposta(r, NUCLEO));
    assert(!pecaExposta(r, PERNA_E));        /* inativa */
    liberarRobo(r);

    r = montarRobo();
    aplicarDano(buscarPeca(r, TRONCO), RES_TRONCO);
    assert(pecaExposta(r, NUCLEO));
    aplicarDano(buscarPeca(r, NUCLEO), RES_NUCLEO);
    assert(!pecaExposta(r, NUCLEO));
    assert(calcularIntegridade(r) == 0);     /* núcleo derruba tudo */
    liberarRobo(r);
}

/* ================= Lista de ações ================= */

TESTE(teste_inserir_remover_trocar)
{
    Acao *a = criarAcao(1, JAB, 0, TRONCO);
    Acao *b = criarAcao(1, BLOQUEIO, 0, NUCLEO);
    Acao *c = criarAcao(1, DIRETO, 1, BASE);
    Acao *lista = NULL;

    assert(inserirAcao(&lista, a, 0));
    assert(inserirAcao(&lista, c, 1));
    assert(inserirAcao(&lista, b, 1));       /* no meio: a, b, c */
    {
        Acao *fora = criarAcao(1, JAB, 0, TRONCO);
        assert(!inserirAcao(&lista, fora, 9));  /* índice além do fim */
        free(fora);
    }
    assert(tamanhoLista(lista) == 3);
    assert(lista == a && a->prox == b && b->prox == c && c->prox == NULL);
    assert(a->posicao == 0 && b->posicao == 1 && c->posicao == 2);
    assert(b->alvo == NUM_PECAS);            /* bloqueio não tem alvo */
    assert(energiaDaLista(lista) == 4);

    assert(trocarAcoes(&lista, 0, 1));       /* vizinhos: b, a, c */
    assert(lista == b && b->prox == a && a->prox == c && c->prox == NULL);
    assert(b->posicao == 0 && a->posicao == 1);

    assert(trocarAcoes(&lista, 2, 0));       /* pontas: c, a, b */
    assert(lista == c && c->prox == a && a->prox == b && b->prox == NULL);
    assert(c->posicao == 0 && b->posicao == 2);
    assert(!trocarAcoes(&lista, 0, 5));

    assert(removerAcao(&lista, 1));          /* remove a: c, b */
    assert(lista == c && c->prox == b && b->posicao == 1);
    assert(!removerAcao(&lista, 7));
    assert(removerAcao(&lista, 0));
    assert(removerAcao(&lista, 0));
    assert(lista == NULL);
}

TESTE(teste_unir_listas)
{
    Acao *a1 = criarAcao(1, JAB, 0, TRONCO);
    Acao *a2 = criarAcao(1, JAB, 0, TRONCO);
    Acao *a3 = criarAcao(1, JAB, 0, TRONCO);
    Acao *b1 = criarAcao(2, JAB, 0, TRONCO);
    Acao *u;

    u = unirListas(listaDe(a1, a2, a3), listaDe(b1, NULL, NULL), 1);
    assert(u == a1 && a1->prox == b1 && b1->prox == a2 && a2->prox == a3 && a3->prox == NULL);
    assert(a1->ordemIntercalacao == 0 && b1->ordemIntercalacao == 1);
    assert(a2->ordemIntercalacao == 2 && a3->ordemIntercalacao == 3);
    liberarLista(u);

    /* rodada par: J2 começa */
    a1 = criarAcao(1, JAB, 0, TRONCO);
    b1 = criarAcao(2, JAB, 0, TRONCO);
    {
        Acao *b2 = criarAcao(2, JAB, 0, TRONCO);
        u = unirListas(listaDe(a1, NULL, NULL), listaDe(b1, b2, NULL), 2);
        assert(u == b1 && b1->prox == a1 && a1->prox == b2 && b2->prox == NULL);
        assert(b1->ordemIntercalacao == 0 && a1->ordemIntercalacao == 1);
    }
    liberarLista(u);
}

TESTE(teste_insertion_sort_estavel)
{
    /* velocidades 5, 8, 5, 8, 3 com ordem 0..4
       esperado: 8(o1), 8(o3), 5(o0), 5(o2), 3(o4) */
    int vel[5] = {5, 8, 5, 8, 3};
    Acao *nos[5];
    Acao *lista = NULL, **fim = &lista;
    int i;
    for (i = 0; i < 5; i++) {
        nos[i] = criarAcao(1 + i % 2, JAB, 0, TRONCO);
        nos[i]->velocidadeEfetiva = vel[i];
        nos[i]->ordemIntercalacao = i;
        *fim = nos[i];
        fim = &nos[i]->prox;
    }
    ordenarPorVelocidade(&lista);
    /* os próprios nós mudaram de lugar (ponteiros), sem cópia de valores */
    assert(lista == nos[1]);
    assert(nos[1]->prox == nos[3]);
    assert(nos[3]->prox == nos[0]);
    assert(nos[0]->prox == nos[2]);
    assert(nos[2]->prox == nos[4]);
    assert(nos[4]->prox == NULL);

    /* ordenar de novo uma lista já ordenada não muda nada */
    ordenarPorVelocidade(&lista);
    assert(lista == nos[1] && nos[2]->prox == nos[4]);

    /* lista vazia e lista de um nó */
    {
        Acao *vazia = NULL;
        ordenarPorVelocidade(&vazia);
        assert(vazia == NULL);
    }
    liberarLista(lista);
}

TESTE(teste_remover_acoes_invalidas)
{
    Peca *r1 = montarRobo(), *r2 = montarRobo();
    Acao *golpeE = criarAcao(1, JAB, FILHO_ESQ, TRONCO);
    Acao *golpeD = criarAcao(1, GANCHO, FILHO_DIR, TRONCO);
    Acao *esq2 = criarAcao(2, ESQUIVA, 0, NUCLEO);
    Acao *bloq2 = criarAcao(2, BLOQUEIO, 0, NUCLEO);
    Acao *fila = unirListas(listaDe(golpeE, golpeD, NULL), listaDe(esq2, bloq2, NULL), 1);
    Acao *canceladas = NULL;

    assert(removerAcoesInvalidas(&fila, r1, r2, &canceladas) == 0);
    aplicarDano(buscarPeca(r1, BRACO_D), RES_BRACO);   /* desliga Punho D de J1 */
    aplicarDano(buscarPeca(r2, BASE), RES_BASE);      /* J2 sem esquiva */
    assert(removerAcoesInvalidas(&fila, r1, r2, &canceladas) == 2);
    assert(tamanhoLista(fila) == 2);
    assert(fila == golpeE && golpeE->prox == bloq2);
    /* lista unida era golpeE, esq2, golpeD, bloq2: canceladas saem nessa ordem */
    assert(canceladas == esq2 && esq2->prox == golpeD && golpeD->prox == NULL);

    liberarLista(fila);
    liberarLista(canceladas);
    liberarRobo(r1);
    liberarRobo(r2);
}

/* ================= Batalha ================= */

TESTE(teste_validacao)
{
    Batalha b;
    const char *motivo;
    assert(iniciarBatalha(&b));
    assert(!validarAcao(&b, 1, JAB, 0, NUCLEO, &motivo));       /* núcleo coberto */
    assert(motivo != NULL);
    assert(programarAcao(&b, 1, GANCHO, 1, TRONCO, 0, &motivo));
    assert(programarAcao(&b, 1, DIRETO, 0, BASE, 5, &motivo));  /* posição limitada ao fim */
    assert(energiaRestante(&b, 1) == 0);
    assert(!programarAcao(&b, 1, BLOQUEIO, 0, NUCLEO, 0, &motivo)); /* sem energia */
    assert(programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL));
    assert(programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL));
    assert(programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL));
    assert(!programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, &motivo)); /* lista cheia */
    aplicarDano(buscarPeca(b.robo[0], PUNHO_E), RES_PUNHO);
    liberarLista(b.programacao[0]);
    b.programacao[0] = NULL;
    assert(!validarAcao(&b, 1, JAB, FILHO_ESQ, TRONCO, &motivo)); /* punho inativo */
    assert(validarAcao(&b, 1, JAB, FILHO_DIR, TRONCO, &motivo));
    aplicarDano(buscarPeca(b.robo[0], BASE), RES_BASE);
    assert(!validarAcao(&b, 1, ESQUIVA, 0, NUCLEO, &motivo));
    assert(velocidadePrevista(&b, 1, JAB, 0) == 8 + 2 + 0);
    encerrarBatalha(&b);
}

TESTE(teste_bloqueio_e_gancho)
{
    Batalha b;
    assert(iniciarBatalha(&b));
    /* J1: Direto (5+2+2 = 9) e Gancho (3+1+2 = 6) no Tronco
       J2: Bloqueio (9+2+2 = 13) executa antes de tudo */
    assert(programarAcao(&b, 1, DIRETO, 0, TRONCO, 0, NULL));
    assert(programarAcao(&b, 1, GANCHO, 1, TRONCO, 1, NULL));
    assert(programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL));
    prepararResolucao(&b);
    assert(b.fila->jogador == 2 && b.fila->tipo == BLOQUEIO);
    assert(b.fila->velocidadeEfetiva == 13);
    {
        Evento ev;
        assert(executarProximaAcao(&b, &ev) && b.bloqueioAtivo[1]);
        assert(executarProximaAcao(&b, &ev));
        assert(ev.acao.tipo == DIRETO && ev.bloqueado && ev.danoCausado == 10);
        assert(executarProximaAcao(&b, &ev));
        assert(ev.acao.tipo == GANCHO && !ev.bloqueado && ev.danoCausado == 35);
        assert(!executarProximaAcao(&b, &ev));
    }
    assert(buscarPeca(b.robo[1], TRONCO)->resistencia == RES_TRONCO - 10 - 35);
    encerrarRodada(&b);
    assert(b.rodada == 2 && b.vencedor == 0);
    encerrarBatalha(&b);
}

TESTE(teste_esquiva_anula_um_golpe)
{
    Batalha b;
    Evento ev;
    assert(iniciarBatalha(&b));
    /* J2 Esquiva (6+2+2 = 10) antes do Direto (5+1+2 = 8) e do Gancho (7) */
    assert(programarAcao(&b, 1, GANCHO, 0, TRONCO, 0, NULL));
    assert(programarAcao(&b, 1, DIRETO, 1, TRONCO, 1, NULL));
    assert(programarAcao(&b, 2, ESQUIVA, 0, NUCLEO, 0, NULL));
    prepararResolucao(&b);
    assert(executarProximaAcao(&b, &ev) && ev.acao.tipo == ESQUIVA);
    assert(executarProximaAcao(&b, &ev) && ev.acao.tipo == DIRETO && ev.esquivado);
    assert(executarProximaAcao(&b, &ev) && ev.acao.tipo == GANCHO && !ev.esquivado);
    assert(ev.danoCausado == 35);
    encerrarRodada(&b);
    encerrarBatalha(&b);
}

TESTE(teste_golpe_no_vazio)
{
    Batalha b;
    Evento ev;
    assert(iniciarBatalha(&b));
    assert(programarAcao(&b, 1, JAB, 0, BRACO_D, 0, NULL));
    aplicarDano(buscarPeca(b.robo[1], BRACO_D), RES_BRACO);   /* alvo some antes */
    prepararResolucao(&b);
    assert(executarProximaAcao(&b, &ev));
    assert(ev.noVazio && ev.danoCausado == 0);
    encerrarRodada(&b);
    encerrarBatalha(&b);
}

TESTE(teste_cancelamento_e_reordenacao_no_meio_da_rodada)
{
    Batalha b;
    Evento ev;
    assert(iniciarBatalha(&b));
    buscarPeca(b.robo[0], BRACO_D)->resistencia = 10;
    buscarPeca(b.robo[1], PERNA_E)->resistencia = 5;
    buscarPeca(b.robo[1], PERNA_D)->resistencia = 5;
    /* J1: Jab na Perna E (12), Jab na Perna D (11), Gancho com Punho D (5) */
    assert(programarAcao(&b, 1, JAB, 0, PERNA_E, 0, NULL));
    assert(programarAcao(&b, 1, JAB, 0, PERNA_D, 1, NULL));
    assert(programarAcao(&b, 1, GANCHO, FILHO_DIR, TRONCO, 2, NULL)); /* energia 5 */
    /* J2: Esquiva (10) e Jab no Braço D de J1 (8+1+2 = 11) */
    assert(programarAcao(&b, 2, ESQUIVA, 0, NUCLEO, 0, NULL));
    assert(programarAcao(&b, 2, JAB, 0, BRACO_D, 1, NULL));
    prepararResolucao(&b);
    /* ordem: J1 Jab 12, J1 Jab 11 (ordem 2) x J2 Jab 11 (ordem 3), J2 Esquiva 10, Gancho 5 */
    assert(b.fila->velocidadeEfetiva == 12);

    assert(executarProximaAcao(&b, &ev));          /* derruba Perna E de J2 */
    assert(ev.pecaDestruida);
    /* J2 perdeu 1 de velocidade: Esquiva vira 9, Jab vira 10 */
    assert(acaoNaPosicao(b.fila, 1)->jogador == 2 && acaoNaPosicao(b.fila, 1)->velocidadeEfetiva == 10);

    assert(executarProximaAcao(&b, &ev));          /* J1 Jab derruba Perna D */
    assert(executarProximaAcao(&b, &ev));          /* J2 Jab derruba Braço D de J1 */
    assert(ev.acao.jogador == 2 && ev.pecaDestruida);
    assert(ev.canceladasNoPasso == 1);             /* Gancho de J1 cancelado */
    assert(b.canceladas != NULL && b.canceladas->tipo == GANCHO);
    assert(tamanhoLista(b.fila) == 1 && b.fila->tipo == ESQUIVA);
    assert(b.fila->velocidadeEfetiva == 6 + 2 + 0);
    encerrarRodada(&b);
    encerrarBatalha(&b);
}

TESTE(teste_luta_completa_ate_vitoria)
{
    Batalha b;
    int alvo;
    assert(iniciarBatalha(&b));
    while (b.vencedor == 0) {
        assert(b.rodada <= MAX_RODADAS);
        /* J1 ataca a Base e, quando o núcleo fica exposto, o Núcleo */
        alvo = pecaExposta(b.robo[1], NUCLEO) ? NUCLEO : BASE;
        assert(programarAcao(&b, 1, GANCHO, FILHO_DIR, (TipoPeca)alvo, 0, NULL));
        assert(programarAcao(&b, 1, DIRETO, FILHO_ESQ, (TipoPeca)alvo, 1, NULL));
        /* J2 só dá Jab na perna de J1 */
        if (pecaAtiva(b.robo[0], PERNA_E)) {
            assert(programarAcao(&b, 2, JAB, FILHO_ESQ, PERNA_E, 0, NULL));
        } else {
            assert(programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL));
        }
        resolverRodada(&b);
    }
    assert(b.vencedor == 1 && b.vitoriaPorNucleo);
    /* rodada 1: Gancho + Direto (55) derrubam a Base (45); rodada 2: o Núcleo (40) */
    assert(b.rodada == 2);
    assert(!pecaAtiva(b.robo[1], NUCLEO));
    assert(calcularIntegridade(b.robo[1]) == 0);
    encerrarBatalha(&b);
}

TESTE(teste_fim_por_rodadas)
{
    Batalha b;
    int r;
    /* só bloqueios: empate */
    assert(iniciarBatalha(&b));
    for (r = 0; r < MAX_RODADAS; r++) {
        programarAcao(&b, 1, BLOQUEIO, 0, NUCLEO, 0, NULL);
        programarAcao(&b, 2, BLOQUEIO, 0, NUCLEO, 0, NULL);
        resolverRodada(&b);
        assert(r == MAX_RODADAS - 1 || b.vencedor == 0);
    }
    assert(b.vencedor == EMPATE && !b.vitoriaPorNucleo);
    encerrarBatalha(&b);

    /* J2 acerta um Jab por rodada: J2 vence pela integridade */
    assert(iniciarBatalha(&b));
    for (r = 0; r < MAX_RODADAS; r++) {
        programarAcao(&b, 1, BLOQUEIO, 0, NUCLEO, 0, NULL);
        programarAcao(&b, 2, JAB, 0, TRONCO, 0, NULL);
        resolverRodada(&b);
    }
    assert(b.vencedor == 2);
    assert(calcularIntegridade(b.robo[0]) == TOTAL_RES - (DANO_JAB / DIVISOR_BLOQUEIO) * MAX_RODADAS);
    encerrarBatalha(&b);
}

int main(void)
{
    printf("Testes do Ringue de Aço\n");
    RODAR(teste_montar_e_buscar);
    RODAR(teste_dano_minimo_zero_e_excedente_perdido);
    RODAR(teste_cascata_tronco);
    RODAR(teste_cascata_braco_e_base);
    RODAR(teste_nucleo_exposto);
    RODAR(teste_inserir_remover_trocar);
    RODAR(teste_unir_listas);
    RODAR(teste_insertion_sort_estavel);
    RODAR(teste_remover_acoes_invalidas);
    RODAR(teste_validacao);
    RODAR(teste_bloqueio_e_gancho);
    RODAR(teste_esquiva_anula_um_golpe);
    RODAR(teste_golpe_no_vazio);
    RODAR(teste_cancelamento_e_reordenacao_no_meio_da_rodada);
    RODAR(teste_luta_completa_ate_vitoria);
    RODAR(teste_fim_por_rodadas);
#ifdef CONTAR_MEMORIA
    printf("Memória conferida: nenhuma alocação sobrando.\n");
#endif
    printf("%d testes passaram.\n", testesRodados);
    return 0;
}
