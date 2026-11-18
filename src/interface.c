/*
 * interface.c
 * Desenho e entrada do Ringue de Aço com raylib.
 * As regras ficam em batalha.c; aqui só se desenha o estado e se lê o
 * teclado e o mouse.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "interface.h"
#include "config.h"

/* ---------- Paleta ---------- */
#define COR_FUNDO        (Color){  20,  20,  28, 255 }
#define COR_PAINEL       (Color){  32,  32,  44, 255 }
#define COR_PAINEL_2     (Color){  46,  46,  62, 255 }
#define COR_LINHA        (Color){  70,  70,  90, 255 }
#define COR_TEXTO        (Color){ 236, 236, 242, 255 }
#define COR_TEXTO_FRACO  (Color){ 150, 150, 168, 255 }
#define COR_DESTAQUE     (Color){ 242, 182,  40, 255 }
#define COR_J1           (Color){ 208,  48,  52, 255 }
#define COR_J2           (Color){  48, 108, 228, 255 }
#define COR_INATIVA      (Color){  92,  92, 100, 255 }
#define COR_ERRO         (Color){ 255,  96,  86, 255 }
#define COR_VIDA_ALTA    (Color){  78, 180,  84, 255 }
#define COR_VIDA_MEDIA   (Color){ 240, 192,  52, 255 }
#define COR_VIDA_BAIXA   (Color){ 226,  78,  62, 255 }

#define PI_F 3.14159265f

/* Posições dos robôs em cada tela */
#define PROG_CHAO   450.0f
#define PROG_ESCALA 0.9f
#define RES_CHAO    650.0f
#define RES_ESCALA  1.0f

/* ================= Entrada ================= */

static bool entradaLiberada = true;

/* liberarEntrada: main.c desliga a entrada das telas quando F1 está aberto. */
void liberarEntrada(bool liberada)
{
    entradaLiberada = liberada;
}

static bool tecla(int k)       { return entradaLiberada && IsKeyPressed(k); }
static bool teclaSegura(int k) { return entradaLiberada && IsKeyDown(k); }
static bool cliqueEsquerdo(void)
{
    return entradaLiberada && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* ================= Auxiliares de desenho ================= */

static Color corDoJogador(int jogador)
{
    return (jogador == 1) ? COR_J1 : COR_J2;
}

/* corVida: verde, amarelo ou vermelho pela vida; cinza se inativa. */
static Color corVida(const Peca *p)
{
    float r;
    if (!p->ativa) {
        return COR_INATIVA;
    }
    r = (float)p->resistencia / (float)p->resistenciaMax;
    if (r > LIMIAR_VIDA_ALTA) {
        return COR_VIDA_ALTA;
    }
    if (r > LIMIAR_VIDA_MEDIA) {
        return COR_VIDA_MEDIA;
    }
    return COR_VIDA_BAIXA;
}

static Color escurecer(Color c, float f)
{
    return (Color){ (unsigned char)(c.r * f), (unsigned char)(c.g * f),
                    (unsigned char)(c.b * f), c.a };
}

static void textoCentralizado(const char *t, float cx, float y, int tam, Color c)
{
    DrawText(t, (int)(cx - MeasureText(t, tam) / 2.0f), (int)y, tam, c);
}

static Vector2 centroDe(Rectangle r)
{
    return (Vector2){ r.x + r.width / 2.0f, r.y + r.height / 2.0f };
}

/* pulso: valor entre 0 e 1 que oscila com o tempo, para destaques piscando. */
static float pulso(void)
{
    return 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
}

/*
 * textoQuebrado: escreve o texto quebrando a linha nos espaços quando passa
 * da largura dada. Devolve a altura ocupada. O(n) no tamanho do texto.
 */
static float textoQuebrado(const char *t, float x, float y, float largura, int tam, Color c)
{
    char linha[192], tentativa[192];
    const char *p = t;
    float yy = y;
    size_t nl, len;

    linha[0] = '\0';
    while (*p != '\0') {
        const char *fim = p;
        while (*fim != '\0' && *fim != ' ') {
            fim++;
        }
        len = (size_t)(fim - p);
        nl = strlen(linha);
        if (nl + len + 2 > sizeof(tentativa)) {
            break;
        }
        memcpy(tentativa, linha, nl);
        if (nl > 0) {
            tentativa[nl++] = ' ';
        }
        memcpy(tentativa + nl, p, len);
        tentativa[nl + len] = '\0';

        if (linha[0] != '\0' && MeasureText(tentativa, tam) > largura) {
            DrawText(linha, (int)x, (int)yy, tam, c);   /* a palavra vai para a próxima linha */
            yy += tam + 4;
            memcpy(linha, p, len);
            linha[len] = '\0';
        } else {
            memcpy(linha, tentativa, nl + len + 1);
        }
        p = fim;
        while (*p == ' ') {
            p++;
        }
    }
    if (linha[0] != '\0') {
        DrawText(linha, (int)x, (int)yy, tam, c);
        yy += tam + 4;
    }
    return yy - y;
}

/* botao: desenha um botão e devolve true se foi clicado. */
static bool botao(Rectangle r, const char *texto, int tam, bool habilitado, bool selecionado)
{
    bool sobre = habilitado && CheckCollisionPointRec(GetMousePosition(), r);
    Color fundo = !habilitado ? (Color){ 40, 40, 50, 255 }
                : selecionado ? COR_DESTAQUE
                : sobre ? (Color){ 78, 78, 98, 255 } : COR_PAINEL_2;
    Color cor = !habilitado ? (Color){ 100, 100, 112, 255 }
              : selecionado ? (Color){ 24, 24, 30, 255 } : COR_TEXTO;
    DrawRectangleRounded(r, 0.2f, 6, fundo);
    if (sobre && !selecionado) {
        DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, COR_DESTAQUE);
    }
    if (texto != NULL) {
        textoCentralizado(texto, r.x + r.width / 2.0f, r.y + (r.height - tam) / 2.0f, tam, cor);
    }
    return sobre && cliqueEsquerdo();
}

/* desenharFundoRingue: fundo, cordas e lona do ringue. */
static void desenharFundoRingue(float chao)
{
    int i;
    DrawRectangleGradientV(0, 0, LARGURA_JANELA, (int)chao, (Color){ 14, 14, 22, 255 },
                           (Color){ 34, 30, 48, 255 });
    /* holofotes (a raylib 6 passou a receber o centro como Vector2) */
#if defined(RAYLIB_VERSION_MAJOR) && RAYLIB_VERSION_MAJOR >= 6
    DrawCircleGradient((Vector2){ LARGURA_JANELA / 2.0f, chao - 260.0f }, 420.0f,
                       Fade((Color){ 255, 240, 200, 255 }, 0.07f), Fade(BLACK, 0.0f));
#else
    DrawCircleGradient(LARGURA_JANELA / 2, (int)(chao - 260), 420.0f,
                       Fade((Color){ 255, 240, 200, 255 }, 0.07f), Fade(BLACK, 0.0f));
#endif
    /* postes e cordas */
    DrawRectangle(90, (int)(chao - 300), 14, 300, (Color){ 60, 60, 72, 255 });
    DrawRectangle(LARGURA_JANELA - 104, (int)(chao - 300), 14, 300, (Color){ 60, 60, 72, 255 });
    for (i = 0; i < 3; i++) {
        float y = chao - 110.0f - i * 75.0f;
        DrawLineEx((Vector2){ 97, y }, (Vector2){ LARGURA_JANELA - 97.0f, y }, 3.0f,
                   Fade((Color){ 190, 190, 205, 255 }, 0.35f));
    }
    /* lona */
    DrawRectangle(0, (int)chao, LARGURA_JANELA, 26, (Color){ 58, 62, 80, 255 });
    DrawRectangle(0, (int)chao + 26, LARGURA_JANELA, ALTURA_JANELA, (Color){ 26, 26, 36, 255 });
    DrawLineEx((Vector2){ 0, chao }, (Vector2){ (float)LARGURA_JANELA, chao }, 2.0f,
               (Color){ 96, 100, 124, 255 });
}

/* desenharBarraIntegridade: barra com a integridade atual do robô. */
static void desenharBarraIntegridade(Rectangle r, int jogador, const Peca *robo)
{
    char texto[48];
    int atual = calcularIntegridade(robo);
    int maximo = calcularIntegridadeMaxima(robo);
    float frac = (maximo > 0) ? (float)atual / (float)maximo : 0.0f;
    DrawRectangleRounded(r, 0.4f, 6, COR_PAINEL);
    if (frac > 0.0f) {
        Rectangle cheio = r;
        cheio.width = r.width * frac;
        if (jogador == 2) {
            cheio.x = r.x + r.width - cheio.width;   /* J2 enche da direita */
        }
        DrawRectangleRounded(cheio, 0.4f, 6, corDoJogador(jogador));
    }
    DrawRectangleRoundedLinesEx(r, 0.4f, 6, 2.0f, COR_LINHA);
    snprintf(texto, sizeof(texto), "J%d  Integridade %d/%d", jogador, atual, maximo);
    if (jogador == 1) {
        DrawText(texto, (int)r.x + 10, (int)(r.y + (r.height - 20) / 2), 20, COR_TEXTO);
    } else {
        DrawText(texto, (int)(r.x + r.width - 10 - MeasureText(texto, 20)),
                 (int)(r.y + (r.height - 20) / 2), 20, COR_TEXTO);
    }
}

/* ================= Robô ================= */

/*
 * caixaLocal: retângulo de cada peça no "espaço do robô": origem nos pés,
 * olhando para a direita, y negativo para cima.
 */
static Rectangle caixaLocal(TipoPeca t)
{
    switch (t) {
    case PERNA_E: return (Rectangle){ -40, -150,  28, 150 };
    case PERNA_D: return (Rectangle){  10, -150,  28, 150 };
    case BASE:    return (Rectangle){ -50, -196, 100,  50 };
    case TRONCO:  return (Rectangle){ -58, -340, 116, 150 };
    case NUCLEO:  return (Rectangle){ -26, -292,  52,  52 };
    case BRACO_E: return (Rectangle){  30, -332,  88,  26 };
    case PUNHO_E: return (Rectangle){ 118, -340,  42,  42 };
    case BRACO_D: return (Rectangle){  36, -268,  78,  26 };
    case PUNHO_D: return (Rectangle){ 114, -276,  42,  42 };
    default:      return (Rectangle){ 0, 0, 0, 0 };
    }
}

/* paraTela: converte um retângulo local em retângulo da tela. */
static Rectangle paraTela(Rectangle local, Vector2 pos, float escala, bool espelhado)
{
    float x = espelhado ? -(local.x + local.width) : local.x;
    return (Rectangle){ pos.x + x * escala, pos.y + local.y * escala,
                        local.width * escala, local.height * escala };
}

/* caixaPeca: retângulo da peça na tela, já com soco e recuo aplicados. */
static Rectangle caixaPeca(TipoPeca t, Vector2 pos, float escala, bool espelhado,
                           const AnimacaoRobo *anim)
{
    Rectangle r = caixaLocal(t);
    if (anim != NULL) {
        r.x -= anim->recuo;
        if (anim->ladoSoco >= 0) {
            if (t == punhoDoLado(anim->ladoSoco)) {
                r.x += anim->avancoSoco;
            } else if (t == bracoDoLado(anim->ladoSoco)) {
                r.width += anim->avancoSoco;   /* o braço estica */
            }
        }
    }
    return paraTela(r, pos, escala, espelhado);
}

/* animacaoPadrao: robô parado, sem efeitos. */
void animacaoPadrao(AnimacaoRobo *anim)
{
    memset(anim, 0, sizeof(*anim));
    anim->pecaPiscando = NUM_PECAS;
    anim->pecaRealcada = NUM_PECAS;
    anim->ladoSoco = -1;
}

typedef struct {
    Vector2 pos;
    float escala;
    bool espelhado;
    Color corTime;
    const AnimacaoRobo *anim;
} ContextoDesenho;

/* desenharPeca: desenha uma peça com cor de vida e contorno do time. */
static void desenharPeca(const Peca *p, const ContextoDesenho *c)
{
    const AnimacaoRobo *anim = c->anim;
    Rectangle r = caixaPeca(p->tipo, c->pos, c->escala, c->espelhado, anim);
    Color preenchimento = corVida(p);
    Color borda = p->ativa ? c->corTime : (Color){ 58, 58, 64, 255 };
    float esp = 3.0f * c->escala;
    float arred = 0.3f;
    bool piscando = anim != NULL && anim->pecaPiscando == p->tipo && anim->piscar > 0.0f
                    && fmod(GetTime() * 14.0, 2.0) < 1.0;

    if (p->tipo == NUCLEO) {
        Vector2 ctr = centroDe(r);
        float raio = r.width / 2.0f;
        DrawCircleV(ctr, raio + esp, borda);
        DrawCircleV(ctr, raio, preenchimento);
        DrawCircleV(ctr, raio * 0.42f, p->ativa ? Fade(WHITE, 0.6f) : (Color){ 70, 70, 76, 255 });
        if (anim != NULL && anim->destacarNucleo && p->ativa) {
            float k = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
            DrawRing(ctr, raio + esp + 2, raio + esp + 5 + 3 * k, 0, 360, 36,
                     Fade(COR_DESTAQUE, 0.5f + 0.5f * k));
        }
        if (piscando) {
            DrawCircleV(ctr, raio + esp, Fade(WHITE, 0.8f * anim->piscar));
        }
        if (anim != NULL && anim->clicavel[NUCLEO]) {
            if (anim->pulsarClicaveis) {
                float k = pulso();
                DrawRing(ctr, raio + esp + 2 + 3 * k, raio + esp + 4 + 5 * k, 0, 360, 36,
                         Fade(COR_DESTAQUE, 0.5f + 0.5f * k));
            } else {
                DrawCircleLinesV(ctr, raio + esp + 3, Fade(COR_DESTAQUE, 0.7f));
            }
        }
        if (anim != NULL && anim->pecaRealcada == NUCLEO) {
            DrawRing(ctr, raio + esp + 2, raio + esp + 6, 0, 360, 36, COR_DESTAQUE);
        }
        return;
    }

    switch (p->tipo) {
    case PUNHO_E: case PUNHO_D: arred = 0.45f; break;
    case BRACO_E: case BRACO_D: arred = 0.6f;  break;
    case TRONCO:                arred = 0.18f; break;
    default:                    arred = 0.3f;  break;
    }

    /* pé, desenhado junto da perna */
    if (p->tipo == PERNA_E || p->tipo == PERNA_D) {
        Rectangle local = caixaLocal(p->tipo);
        Rectangle pe = { local.x - 4 - (anim ? anim->recuo : 0), -16, local.width + 22, 16 };
        Rectangle peTela = paraTela(pe, c->pos, c->escala, c->espelhado);
        DrawRectangleRounded(peTela, 0.4f, 6, escurecer(preenchimento, 0.75f));
        DrawRectangleRoundedLinesEx(peTela, 0.4f, 6, esp, borda);
    }

    DrawRectangleRounded(r, arred, 8, preenchimento);
    DrawRectangleRoundedLinesEx(r, arred, 8, esp, borda);

    /* detalhes de chapa no tronco */
    if (p->tipo == TRONCO) {
        Color linha = Fade(BLACK, 0.25f);
        DrawLineEx((Vector2){ r.x + 10 * c->escala, r.y + r.height * 0.22f },
                   (Vector2){ r.x + r.width - 10 * c->escala, r.y + r.height * 0.22f },
                   2.0f, linha);
        DrawLineEx((Vector2){ r.x + 10 * c->escala, r.y + r.height * 0.85f },
                   (Vector2){ r.x + r.width - 10 * c->escala, r.y + r.height * 0.85f },
                   2.0f, linha);
    }
    /* rachadura nas peças inativas */
    if (!p->ativa) {
        DrawLineEx((Vector2){ r.x + r.width * 0.25f, r.y + r.height * 0.2f },
                   (Vector2){ r.x + r.width * 0.55f, r.y + r.height * 0.55f },
                   2.0f, (Color){ 40, 40, 44, 255 });
        DrawLineEx((Vector2){ r.x + r.width * 0.55f, r.y + r.height * 0.55f },
                   (Vector2){ r.x + r.width * 0.75f, r.y + r.height * 0.8f },
                   2.0f, (Color){ 40, 40, 44, 255 });
    }
    if (piscando) {
        DrawRectangleRounded(r, arred, 8, Fade(WHITE, 0.85f * anim->piscar));
    }
    if (anim != NULL && anim->clicavel[p->tipo]) {
        /* no passo "clique no alvo" o contorno pulsa para chamar a atenção */
        float k = anim->pulsarClicaveis ? pulso() : 0.0f;
        float folga = 3.0f + 3.0f * k;
        Rectangle fora = { r.x - folga, r.y - folga, r.width + 2 * folga, r.height + 2 * folga };
        DrawRectangleRoundedLinesEx(fora, arred, 8, 1.5f + 2.0f * k,
                                    Fade(COR_DESTAQUE, anim->pulsarClicaveis ? 0.45f + 0.55f * k : 0.7f));
    }
    if (anim != NULL && anim->pecaRealcada == p->tipo) {
        Rectangle fora = { r.x - 4, r.y - 4, r.width + 8, r.height + 8 };
        DrawRectangleRoundedLinesEx(fora, arred, 8, 4.0f, COR_DESTAQUE);
    }
}

/*
 * desenharPecaRec: percorre a árvore em pós-ordem (filhos antes do pai).
 * Assim o punho sai atrás do braço, as pernas atrás da Base e o Núcleo,
 * que é a raiz, é desenhado por último, por cima do Tronco. O(n).
 */
static void desenharPecaRec(const Peca *p, const ContextoDesenho *c)
{
    if (p == NULL) {
        return;
    }
    desenharPecaRec(p->esq, c);
    desenharPecaRec(p->dir, c);
    desenharPeca(p, c);
}

/*
 * desenharRobo: desenha o robô percorrendo a árvore de peças. A cabeça e o
 * escudo são só enfeites (não são peças). "espelhado" vira o robô para a
 * esquerda (robô do J2). O(n).
 */
void desenharRobo(const Peca *raiz, Vector2 pos, float escala, bool espelhado,
                  Color corTime, const AnimacaoRobo *anim)
{
    ContextoDesenho c;
    AnimacaoRobo padrao;
    float recuo;
    bool ligado = raiz != NULL && raiz->ativa;
    Rectangle pescoco, cabeca, visor, olho;

    if (anim == NULL) {
        animacaoPadrao(&padrao);
        anim = &padrao;
    }
    recuo = anim->recuo;
    c.pos = pos;
    c.escala = escala;
    c.espelhado = espelhado;
    c.corTime = corTime;
    c.anim = anim;

    /* sombra no chão */
    DrawEllipse((int)pos.x, (int)pos.y + 4, 90 * escala, 10 * escala, Fade(BLACK, 0.35f));

    /* pescoço e cabeça (enfeite) */
    pescoco = paraTela((Rectangle){ -10 - recuo, -352, 20, 16 }, pos, escala, espelhado);
    cabeca  = paraTela((Rectangle){ -26 - recuo, -394, 52, 46 }, pos, escala, espelhado);
    visor   = paraTela((Rectangle){   2 - recuo, -380, 26, 13 }, pos, escala, espelhado);
    olho    = paraTela((Rectangle){  15 - recuo, -378, 9,   9 }, pos, escala, espelhado);
    DrawRectangleRec(pescoco, (Color){ 70, 70, 80, 255 });
    DrawRectangleRounded(cabeca, 0.35f, 8, ligado ? corTime : COR_INATIVA);
    DrawRectangleRoundedLinesEx(cabeca, 0.35f, 8, 2.0f * escala, escurecer(corTime, 0.55f));
    DrawRectangleRounded(visor, 0.4f, 6, (Color){ 18, 18, 24, 255 });
    DrawRectangleRounded(olho, 0.5f, 6, ligado ? COR_DESTAQUE : (Color){ 60, 60, 66, 255 });

    desenharPecaRec(raiz, &c);

    /* escudo do Bloqueio */
    if (anim->escudo) {
        float k = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        Rectangle esc = paraTela((Rectangle){ 172 - recuo, -372, 14, 196 }, pos, escala, espelhado);
        DrawRectangleRounded(esc, 0.6f, 8, Fade(SKYBLUE, 0.25f + 0.15f * k));
        DrawRectangleRoundedLinesEx(esc, 0.6f, 8, 2.0f, Fade(SKYBLUE, 0.8f));
    }
}

/*
 * pecaNoPonto: qual peça do robô está sob o ponto (mouse). Testa da frente
 * para trás. Com "soExpostas", ignora peças que não podem ser atingidas
 * (assim o clique no Núcleo coberto vai para o Tronco).
 * Devolve NUM_PECAS se nenhuma.
 */
TipoPeca pecaNoPonto(Peca *raiz, Vector2 pos, float escala, bool espelhado,
                     Vector2 ponto, bool soExpostas)
{
    static const TipoPeca ordem[NUM_PECAS] = {
        NUCLEO, BASE, PERNA_D, PERNA_E, TRONCO, BRACO_D, PUNHO_D, BRACO_E, PUNHO_E
    };
    int i;
    for (i = 0; i < NUM_PECAS; i++) {
        TipoPeca t = ordem[i];
        Rectangle r;
        bool dentro;
        if (soExpostas && !pecaExposta(raiz, t)) {
            continue;
        }
        r = caixaPeca(t, pos, escala, espelhado, NULL);
        if (t == NUCLEO) {
            dentro = CheckCollisionPointCircle(ponto, centroDe(r), r.width / 2.0f + 4);
        } else {
            dentro = CheckCollisionPointRec(ponto, r);
        }
        if (dentro) {
            return t;
        }
    }
    return NUM_PECAS;
}

/* ================= Menu e Como jogar ================= */

/*
 * menuPrincipal: Nova luta, Como jogar, Sair. Setas + ENTER ou mouse.
 * Devolve a próxima tela.
 */
Tela menuPrincipal(int *opcao)
{
    static const char *itens[3] = { "Nova luta", "Como jogar", "Sair" };
    static const Tela destino[3] = { TELA_PROGRAMACAO, TELA_COMO_JOGAR, TELA_SAIR };
    Tela proxima = TELA_MENU;
    int i;

    if (tecla(KEY_DOWN) || tecla(KEY_S)) {
        *opcao = (*opcao + 1) % 3;
    }
    if (tecla(KEY_UP) || tecla(KEY_W)) {
        *opcao = (*opcao + 2) % 3;
    }
    if (tecla(KEY_ENTER) || tecla(KEY_KP_ENTER) || tecla(KEY_SPACE)) {
        proxima = destino[*opcao];
    }

    desenharFundoRingue(620.0f);
    {
        /* robôs de enfeite: montados e liberados a cada quadro */
        Peca *demo = montarRobo();
        if (demo != NULL) {
            desenharRobo(demo, (Vector2){ 250, 620 }, 0.9f, false, COR_J1, NULL);
            desenharRobo(demo, (Vector2){ LARGURA_JANELA - 250.0f, 620 }, 0.9f, true, COR_J2, NULL);
            liberarRobo(demo);
        }
    }

    textoCentralizado("RINGUE DE AÇO", LARGURA_JANELA / 2.0f + 4, 94, 80, Fade(BLACK, 0.6f));
    textoCentralizado("RINGUE DE AÇO", LARGURA_JANELA / 2.0f, 90, 80, COR_DESTAQUE);
    textoCentralizado("Duelo de robôs por turnos para 2 jogadores", LARGURA_JANELA / 2.0f, 185,
                      20, COR_TEXTO);

    for (i = 0; i < 3; i++) {
        Rectangle r = { LARGURA_JANELA / 2.0f - 170, 280.0f + i * 74, 340, 56 };
        if (CheckCollisionPointRec(GetMousePosition(), r)) {
            *opcao = i;
        }
        if (botao(r, itens[i], 30, true, *opcao == i)) {
            proxima = destino[i];
        }
    }

    textoCentralizado("Setas e ENTER, ou clique", LARGURA_JANELA / 2.0f, 520, 20, COR_TEXTO_FRACO);
    textoCentralizado("AED - CESAR School - Paulo Portella, Gustavo Laporte e Gabriel Ferraz",
                      LARGURA_JANELA / 2.0f, 690, 10, COR_TEXTO_FRACO);
    return proxima;
}

/* telaComoJogar: regras resumidas e a tabela de ações. */
Tela telaComoJogar(void)
{
    static const char *regras[] = {
        "- Cada robô é uma árvore de 9 peças. Peça zerada desliga tudo abaixo dela:",
        "  Tronco leva braços e punhos, Braço leva o punho, Base leva as pernas.",
        "- A cada rodada cada jogador programa em segredo de 1 a 3 ações, com 5 de energia.",
        "- As listas dos dois são intercaladas e ordenadas por velocidade (Insertion Sort).",
        "  A mais rápida executa primeiro. Empate: quem veio antes na intercalação.",
        "- Velocidade = base + posição na sua lista (1a +2, 2a +1, 3a +0) + 1 por perna ativa.",
        "- O Núcleo só pode ser atingido se o Tronco ou a Base caírem. Núcleo destruído = vitória.",
        "- Depois de 6 rodadas vence quem tiver mais integridade.",
        "- Bloqueio segura Jab e Direto. Gancho quebra o Bloqueio. Esquiva pega o Gancho (é lento).",
    };
    static const char *colunas[] = { "Ação", "Energia", "Veloc.", "Dano", "Efeito" };
    static const float xs[] = { 80, 250, 380, 500, 610 };
    static const char *efeitos[NUM_ACOES] = {
        "Rápido e barato. Usa o punho escolhido.",
        "Golpe médio. Usa o punho escolhido.",
        "Lento e forte. Ignora o Bloqueio.",
        "Golpes recebidos depois dele: metade do dano.",
        "Exige Base. Anula o próximo golpe recebido.",
    };
    char buf[16];
    int i, t;
    float y;

    DrawRectangle(0, 0, LARGURA_JANELA, ALTURA_JANELA, COR_FUNDO);
    textoCentralizado("COMO JOGAR", LARGURA_JANELA / 2.0f, 24, 40, COR_DESTAQUE);

    y = 84;
    for (i = 0; i < (int)(sizeof(regras) / sizeof(regras[0])); i++) {
        DrawText(regras[i], 70, (int)y, 20, COR_TEXTO);
        y += 28;
    }

    y += 14;
    DrawRectangleRounded((Rectangle){ 60, y - 8, LARGURA_JANELA - 120.0f, 214 }, 0.06f, 6, COR_PAINEL);
    for (i = 0; i < 5; i++) {
        DrawText(colunas[i], (int)xs[i], (int)y, 20, COR_DESTAQUE);
    }
    y += 32;
    for (t = 0; t < NUM_ACOES; t++) {
        DrawText(nomeAcao((TipoAcao)t), (int)xs[0], (int)y, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "%d", custoAcao((TipoAcao)t));
        DrawText(buf, (int)xs[1], (int)y, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "%d", velocidadeBase((TipoAcao)t));
        DrawText(buf, (int)xs[2], (int)y, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "%d", danoAcao((TipoAcao)t));
        DrawText(danoAcao((TipoAcao)t) > 0 ? buf : "-", (int)xs[3], (int)y, 20, COR_TEXTO);
        DrawText(efeitos[t], (int)xs[4], (int)y, 20, COR_TEXTO_FRACO);
        y += 34;
    }

    DrawText("Controles: 1-5 ação | setas esq/dir punho | clique na peça do rival | "
             "setas cima/baixo posição", 70, 600, 20, COR_TEXTO_FRACO);
    DrawText("SHIFT + cima/baixo troca | DEL remove | ENTER confirma | F1 depuração",
             70, 628, 20, COR_TEXTO_FRACO);

    if (botao((Rectangle){ LARGURA_JANELA / 2.0f - 120, 664, 240, 44 }, "Voltar (ESC)", 20, true, false)
        || tecla(KEY_ESCAPE) || tecla(KEY_SPACE) || tecla(KEY_BACKSPACE) || tecla(KEY_ENTER)) {
        return TELA_MENU;
    }
    return TELA_COMO_JOGAR;
}

/* ================= Programação ================= */

/* reiniciarProgramacao: estado inicial da tela de um jogador. */
void reiniciarProgramacao(EstadoProgramacao *ep)
{
    ep->acao = JAB;
    ep->lado = FILHO_DIR;
    ep->cursor = 0;
    ep->escolheuAcao = false;
    ep->mensagem[0] = '\0';
    ep->tempoMensagem = 0.0f;
}

static void avisar(EstadoProgramacao *ep, const char *msg)
{
    snprintf(ep->mensagem, sizeof(ep->mensagem), "%s", msg ? msg : "Ação inválida");
    ep->tempoMensagem = 2.5f;
}

/* acaoDisponivel: se o botão da ação deve ficar habilitado (sem olhar o alvo). */
static bool acaoDisponivel(Batalha *b, int jogador, TipoAcao t)
{
    Peca *r = b->robo[jogador - 1];
    if (tamanhoLista(b->programacao[jogador - 1]) >= MAX_ACOES_POR_JOGADOR) {
        return false;
    }
    if (custoAcao(t) > energiaRestante(b, jogador)) {
        return false;
    }
    if (ehGolpe(t)) {
        return pecaAtiva(r, PUNHO_E) || pecaAtiva(r, PUNHO_D);
    }
    if (t == ESQUIVA) {
        return pecaAtiva(r, BASE);
    }
    return true;
}

static Rectangle retanguloSlot(int i)
{
    return (Rectangle){ 492, 514.0f + i * 62, 420, 56 };
}

/* descricaoAcao: "Jab (E) > Tronco" ou só o nome. */
static void descricaoAcao(const Acao *a, char *buf, int tam)
{
    if (ehGolpe(a->tipo)) {
        snprintf(buf, tam, "%s (%s) > %s", nomeAcao(a->tipo),
                 a->lado == FILHO_ESQ ? "E" : "D", nomePeca(a->alvo));
    } else {
        snprintf(buf, tam, "%s", nomeAcao(a->tipo));
    }
}

/* desenharCabecalho: barra do topo com rodada, título e placar. */
static void desenharCabecalho(int rodada, const char *titulo, Color corTitulo, const Placar *placar)
{
    char buf[64];
    DrawRectangle(0, 0, LARGURA_JANELA, 44, COR_PAINEL);
    snprintf(buf, sizeof(buf), "RODADA %d/%d", rodada, MAX_RODADAS);
    DrawText(buf, 20, 12, 20, COR_TEXTO);
    textoCentralizado(titulo, LARGURA_JANELA / 2.0f, 12, 20, corTitulo);
    if (placar != NULL) {
        snprintf(buf, sizeof(buf), "Placar  J1 %d x %d J2", placar->vitorias[0], placar->vitorias[1]);
        DrawText(buf, LARGURA_JANELA - 20 - MeasureText(buf, 20), 12, 20, COR_TEXTO);
    }
}

/* ================= Guia passo a passo ================= */

/* Passos do guia e o estado de cada um ao desenhar */
enum { PASSO_ACAO = 1, PASSO_PUNHO, PASSO_ALVO, PASSO_CONFIRMAR };
enum { GUIA_PENDENTE, GUIA_LIVRE, GUIA_ATUAL, GUIA_FEITO };

/* Painel do guia no meio do ringue, entre os dois robôs */
#define GUIA_X 488.0f
#define GUIA_Y 132.0f
#define GUIA_L 304.0f
#define GUIA_A 308.0f

/* nenhumaAcaoDisponivel: lista cheia ou sem energia para qualquer ação. */
static bool nenhumaAcaoDisponivel(Batalha *b, int jogador)
{
    int t;
    for (t = 0; t < NUM_ACOES; t++) {
        if (acaoDisponivel(b, jogador, (TipoAcao)t)) {
            return false;
        }
    }
    return true;
}

/*
 * passoAtualDoGuia: em que passo o jogador está.
 *   1 escolher a ação, 3 clicar no alvo (ou ESPAÇO na defesa),
 *   4 confirmar quando não cabe mais nada. O punho (2) já vem escolhido.
 */
static int passoAtualDoGuia(Batalha *b, int jogador, const EstadoProgramacao *ep)
{
    if (tamanhoLista(b->programacao[jogador - 1]) >= MAX_ACOES_POR_JOGADOR
        || nenhumaAcaoDisponivel(b, jogador)) {
        return PASSO_CONFIRMAR;
    }
    if (!ep->escolheuAcao || !acaoDisponivel(b, jogador, ep->acao)) {
        return PASSO_ACAO;
    }
    return PASSO_ALVO;
}

/* explicarAcao: uma frase sobre para que serve a ação. */
static void explicarAcao(TipoAcao t, char *buf, int tam)
{
    switch (t) {
    case JAB:
        snprintf(buf, tam, "Jab: rápido e barato, tira %d. O Bloqueio corta pela metade.", danoAcao(JAB));
        break;
    case DIRETO:
        snprintf(buf, tam, "Direto: o meio-termo, tira %d. O Bloqueio corta pela metade.", danoAcao(DIRETO));
        break;
    case GANCHO:
        snprintf(buf, tam, "Gancho: lento e forte, tira %d e fura o Bloqueio. A Esquiva anula.",
                 danoAcao(GANCHO));
        break;
    case BLOQUEIO:
        snprintf(buf, tam, "Bloqueio: sai rápido e, até o fim da rodada, Jab e Direto tiram só a metade.");
        break;
    case ESQUIVA:
        snprintf(buf, tam, "Esquiva: o próximo golpe do rival erra. Boa contra o Gancho, que é lento.");
        break;
    default:
        buf[0] = '\0';
        break;
    }
}

/* motivoIndisponivel: por que a ação escolhida não pode entrar agora. */
static void motivoIndisponivel(Batalha *b, int jogador, TipoAcao t, char *buf, int tam)
{
    Peca *r = b->robo[jogador - 1];
    if (custoAcao(t) > energiaRestante(b, jogador)) {
        snprintf(buf, tam, "Sem energia: %s custa %d e você tem %d. Escolha outra ação.",
                 nomeAcao(t), custoAcao(t), energiaRestante(b, jogador));
    } else if (ehGolpe(t) && !pecaAtiva(r, PUNHO_E) && !pecaAtiva(r, PUNHO_D)) {
        snprintf(buf, tam, "Seu robô está sem punhos: só dá para defender.");
    } else if (t == ESQUIVA && !pecaAtiva(r, BASE)) {
        snprintf(buf, tam, "Sem a Base, o robô não consegue esquivar.");
    } else {
        buf[0] = '\0';
    }
}

/* desenharPassoGuia: bolinha com o número (ou um visto) e o texto do passo. */
static void desenharPassoGuia(float x, float y, int numero, const char *texto, int estado)
{
    Vector2 c = { x + 14, y + 14 };
    Color corTexto;
    char num[4];

    snprintf(num, sizeof(num), "%d", numero);
    if (estado == GUIA_FEITO) {
        DrawCircleV(c, 13, COR_VIDA_ALTA);
        DrawLineEx((Vector2){ c.x - 6, c.y }, (Vector2){ c.x - 2, c.y + 5 }, 3.0f, WHITE);
        DrawLineEx((Vector2){ c.x - 2, c.y + 5 }, (Vector2){ c.x + 7, c.y - 6 }, 3.0f, WHITE);
        corTexto = COR_TEXTO;
    } else if (estado == GUIA_ATUAL) {
        float k = pulso();
        DrawCircleV(c, 14 + 5 * k, Fade(COR_DESTAQUE, 0.35f * (1.0f - k) + 0.1f));
        DrawCircleV(c, 13, COR_DESTAQUE);
        textoCentralizado(num, c.x, c.y - 10, 20, BLACK);
        corTexto = COR_DESTAQUE;
    } else {
        Color cor = (estado == GUIA_LIVRE) ? COR_TEXTO : COR_LINHA;
        DrawCircleLinesV(c, 13, cor);
        textoCentralizado(num, c.x, c.y - 10, 20, cor);
        corTexto = (estado == GUIA_LIVRE) ? COR_TEXTO : COR_TEXTO_FRACO;
    }
    DrawText(texto, (int)(x + 36), (int)(y + 4), 20, corTexto);
}

/*
 * desenharGuia: painel "PASSO A PASSO" no meio do ringue. Mostra os 4
 * passos para montar a lista, destaca o passo atual e explica a ação
 * escolhida (ou por que ela não pode entrar).
 */
static void desenharGuia(Batalha *b, int jogador, const EstadoProgramacao *ep, int passo)
{
    Rectangle painel = { GUIA_X, GUIA_Y, GUIA_L, GUIA_A };
    float x = GUIA_X + 14, y = GUIA_Y + 12;
    int n = tamanhoLista(b->programacao[jogador - 1]);
    bool golpe = ehGolpe(ep->acao);
    bool montando = (passo == PASSO_ALVO);
    char buf[128];

    DrawRectangleRounded(painel, 0.06f, 8, Fade(COR_PAINEL, 0.94f));
    DrawRectangleRoundedLinesEx(painel, 0.06f, 8, 2.0f, COR_LINHA);
    DrawText("PASSO A PASSO", (int)x, (int)y, 20, COR_DESTAQUE);
    y += 34;

    /* 1: ação */
    if (montando) {
        snprintf(buf, sizeof(buf), "Ação: %s", nomeAcao(ep->acao));
    } else {
        snprintf(buf, sizeof(buf), "Escolha a ação (1-5)");
    }
    desenharPassoGuia(x, y, 1, buf,
                      passo == PASSO_ACAO ? GUIA_ATUAL : montando ? GUIA_FEITO : GUIA_PENDENTE);
    y += 36;

    /* 2: punho (já vem escolhido; as setas trocam) */
    if (montando && !golpe) {
        snprintf(buf, sizeof(buf), "Punho: não usa");
    } else {
        snprintf(buf, sizeof(buf), "Punho %s (setas)", ep->lado == FILHO_ESQ ? "esquerdo" : "direito");
    }
    desenharPassoGuia(x, y, 2, buf, montando ? GUIA_FEITO : GUIA_PENDENTE);
    y += 36;

    /* 3: alvo ou ESPAÇO */
    desenharPassoGuia(x, y, 3, (montando && !golpe) ? "Aperte ESPAÇO" : "Clique no alvo",
                      montando ? GUIA_ATUAL : GUIA_PENDENTE);
    y += 36;

    /* 4: confirmar */
    desenharPassoGuia(x, y, 4, "Confirme (ENTER)",
                      passo == PASSO_CONFIRMAR ? GUIA_ATUAL : (n > 0 ? GUIA_LIVRE : GUIA_PENDENTE));
    y += 42;

    DrawLineEx((Vector2){ GUIA_X + 14, y }, (Vector2){ GUIA_X + GUIA_L - 14, y }, 1.0f, COR_LINHA);
    y += 10;

    /* explicação embaixo dos passos */
    if (passo == PASSO_CONFIRMAR) {
        textoQuebrado(n >= MAX_ACOES_POR_JOGADOR
                          ? "Lista cheia! Aperte ENTER para confirmar ou mude a ordem antes."
                          : "Acabou a energia. Aperte ENTER para confirmar.",
                      x, y, GUIA_L - 28, 20, COR_TEXTO);
        return;
    }
    motivoIndisponivel(b, jogador, ep->acao, buf, sizeof(buf));
    if (buf[0] != '\0' && ep->escolheuAcao) {
        textoQuebrado(buf, x, y, GUIA_L - 28, 20, COR_ERRO);
        return;
    }
    explicarAcao(ep->acao, buf, sizeof(buf));
    textoQuebrado(buf, x, y, GUIA_L - 28, 20, COR_TEXTO_FRACO);
}

/*
 * telaProgramacao: tela secreta em que o jogador monta a sua lista de até
 * 3 ações. Escolhe a ação (1-5), o punho (setas esq/dir) e clica na peça
 * do rival. Pode inserir numa posição, remover e trocar ações de lugar.
 * Devolve TELA_TROCA quando o jogador confirma com ENTER.
 */
Tela telaProgramacao(Batalha *b, int jogador, EstadoProgramacao *ep, const Placar *placar)
{
    int idx = jogador - 1;
    Acao **lista = &b->programacao[idx];
    Peca *meu = b->robo[idx];
    Peca *rival = b->robo[1 - idx];
    Vector2 posicoes[2] = { { 330, PROG_CHAO }, { 950, PROG_CHAO } };
    Vector2 posRival = posicoes[1 - idx];
    bool rivalEspelhado = (idx == 0);
    Vector2 mouse = GetMousePosition();
    TipoPeca sobMouse = NUM_PECAS;
    const char *motivo = NULL;
    bool confirmar = false;
    bool shift = teclaSegura(KEY_LEFT_SHIFT) || teclaSegura(KEY_RIGHT_SHIFT);
    int n, maxCursor, t, i, passo;
    char buf[96];
    AnimacaoRobo anim[2];

    if (ep->tempoMensagem > 0.0f) {
        ep->tempoMensagem -= GetFrameTime();
    }
    /* se o punho escolhido caiu, passa para o outro */
    if (!pecaAtiva(meu, punhoDoLado(ep->lado)) && pecaAtiva(meu, punhoDoLado(1 - ep->lado))) {
        ep->lado = 1 - ep->lado;
    }

    /* ---------- teclado ---------- */
    for (t = 0; t < NUM_ACOES; t++) {
        if (tecla(KEY_ONE + t) || tecla(KEY_KP_1 + t)) {
            ep->acao = (TipoAcao)t;
            ep->escolheuAcao = true;
        }
    }
    if (tecla(KEY_LEFT) || tecla(KEY_A)) {
        ep->lado = FILHO_ESQ;
    }
    if (tecla(KEY_RIGHT) || tecla(KEY_D)) {
        ep->lado = FILHO_DIR;
    }
    n = tamanhoLista(*lista);
    if (tecla(KEY_UP) || tecla(KEY_W)) {
        if (shift) {
            if (ep->cursor > 0 && ep->cursor < n) {
                trocarAcoes(lista, ep->cursor, ep->cursor - 1);
                ep->cursor--;
            }
        } else if (ep->cursor > 0) {
            ep->cursor--;
        }
    }
    if (tecla(KEY_DOWN) || tecla(KEY_S)) {
        if (shift) {
            if (ep->cursor + 1 < n) {
                trocarAcoes(lista, ep->cursor, ep->cursor + 1);
                ep->cursor++;
            }
        } else {
            ep->cursor++;
        }
    }
    if ((tecla(KEY_DELETE) || tecla(KEY_BACKSPACE)) && ep->cursor < n) {
        removerAcao(lista, ep->cursor);
    }
    if (tecla(KEY_SPACE) && !ehGolpe(ep->acao)) {
        if (programarAcao(b, jogador, ep->acao, ep->lado, NUM_PECAS, ep->cursor, &motivo)) {
            ep->cursor++;
            ep->escolheuAcao = false;   /* guia volta ao passo 1 */
        } else {
            avisar(ep, motivo);
        }
    }
    if (tecla(KEY_ENTER) || tecla(KEY_KP_ENTER)) {
        confirmar = true;
    }

    /* ---------- mouse no robô rival ---------- */
    if (ehGolpe(ep->acao) && entradaLiberada) {
        sobMouse = pecaNoPonto(rival, posRival, PROG_ESCALA, rivalEspelhado, mouse, true);
        if (sobMouse != NUM_PECAS && cliqueEsquerdo()) {
            if (programarAcao(b, jogador, ep->acao, ep->lado, sobMouse, ep->cursor, &motivo)) {
                ep->cursor++;
                ep->escolheuAcao = false;
            } else {
                avisar(ep, motivo);
            }
        }
    }

    /* ---------- mouse na lista ---------- */
    n = tamanhoLista(*lista);
    for (i = 0; i < MAX_ACOES_POR_JOGADOR; i++) {
        Rectangle s = retanguloSlot(i);
        Rectangle bCima = { s.x + s.width - 124, s.y + 13, 36, 30 };
        Rectangle bBaixo = { s.x + s.width - 84, s.y + 13, 36, 30 };
        Rectangle bTirar = { s.x + s.width - 44, s.y + 13, 36, 30 };
        if (i < n && CheckCollisionPointRec(mouse, bCima)) {
            if (cliqueEsquerdo() && i > 0) {
                trocarAcoes(lista, i, i - 1);
                ep->cursor = i - 1;
            }
        } else if (i < n && CheckCollisionPointRec(mouse, bBaixo)) {
            if (cliqueEsquerdo() && i + 1 < n) {
                trocarAcoes(lista, i, i + 1);
                ep->cursor = i + 1;
            }
        } else if (i < n && CheckCollisionPointRec(mouse, bTirar)) {
            if (cliqueEsquerdo()) {
                removerAcao(lista, i);
                ep->cursor = i;
            }
        } else if (CheckCollisionPointRec(mouse, s) && cliqueEsquerdo()) {
            ep->cursor = i;
        }
    }

    /* cursor sempre entre 0 e o primeiro espaço vazio (máximo 2) */
    n = tamanhoLista(*lista);
    maxCursor = (n < MAX_ACOES_POR_JOGADOR) ? n : MAX_ACOES_POR_JOGADOR - 1;
    if (ep->cursor > maxCursor) {
        ep->cursor = maxCursor;
    }
    if (ep->cursor < 0) {
        ep->cursor = 0;
    }

    /* ================= desenho ================= */
    passo = passoAtualDoGuia(b, jogador, ep);
    desenharFundoRingue(PROG_CHAO);
    snprintf(buf, sizeof(buf), "PROGRAMAÇÃO SECRETA - JOGADOR %d", jogador);
    desenharCabecalho(b->rodada, buf, corDoJogador(jogador), placar);
    desenharBarraIntegridade((Rectangle){ 40, 54, 480, 26 }, 1, b->robo[0]);
    desenharBarraIntegridade((Rectangle){ 760, 54, 480, 26 }, 2, b->robo[1]);

    animacaoPadrao(&anim[0]);
    animacaoPadrao(&anim[1]);
    for (i = 0; i < 2; i++) {
        anim[i].destacarNucleo = pecaExposta(b->robo[i], NUCLEO);
    }
    if (ehGolpe(ep->acao)) {
        anim[idx].pecaRealcada = punhoDoLado(ep->lado);
        for (t = 0; t < NUM_PECAS; t++) {
            anim[1 - idx].clicavel[t] = pecaExposta(rival, (TipoPeca)t);
        }
        anim[1 - idx].pecaRealcada = sobMouse;
        anim[1 - idx].pulsarClicaveis = (passo == PASSO_ALVO);
    }
    desenharRobo(b->robo[0], posicoes[0], PROG_ESCALA, false, COR_J1, &anim[0]);
    desenharRobo(b->robo[1], posicoes[1], PROG_ESCALA, true, COR_J2, &anim[1]);
    desenharGuia(b, jogador, ep, passo);

    /* marcadores numerados nas peças que a sua lista já mira */
    {
        int naPeca[NUM_PECAS] = { 0 };
        Acao *a;
        for (a = *lista; a != NULL; a = a->prox) {
            if (ehGolpe(a->tipo) && a->alvo < NUM_PECAS) {
                Rectangle r = caixaPeca(a->alvo, posRival, PROG_ESCALA, rivalEspelhado, NULL);
                Vector2 c = centroDe(r);
                c.x += naPeca[a->alvo] * 22.0f - 6.0f;
                naPeca[a->alvo]++;
                DrawCircleV(c, 12, COR_DESTAQUE);
                DrawCircleLinesV(c, 12, BLACK);
                snprintf(buf, sizeof(buf), "%d", a->posicao + 1);
                textoCentralizado(buf, c.x, c.y - 10, 20, BLACK);
            }
        }
    }

    /* dica sobre o mouse */
    if (sobMouse != NUM_PECAS) {
        snprintf(buf, sizeof(buf), "%s com Punho %s %s %s", nomeAcao(ep->acao),
                 ep->lado == FILHO_ESQ ? "E" : "D", preposicaoPeca(sobMouse), nomePeca(sobMouse));
        DrawRectangleRounded((Rectangle){ mouse.x + 14, mouse.y + 14, MeasureText(buf, 20) + 16.0f, 30 },
                             0.3f, 6, Fade(BLACK, 0.8f));
        DrawText(buf, (int)mouse.x + 22, (int)mouse.y + 19, 20, COR_DESTAQUE);
    }

    /* ---------- painel inferior ---------- */
    DrawRectangle(0, (int)PROG_CHAO + 26, LARGURA_JANELA, ALTURA_JANELA, COR_FUNDO);
    DrawLine(0, (int)PROG_CHAO + 26, LARGURA_JANELA, (int)PROG_CHAO + 26, COR_LINHA);

    /* coluna 1: ação e punho */
    DrawText("AÇÃO (teclas 1-5)", 20, 488, 20, COR_DESTAQUE);
    if (passo == PASSO_ACAO) {
        /* guia: moldura piscando em volta dos botões de ação */
        float k = pulso();
        DrawRectangleRoundedLinesEx((Rectangle){ 14, 508, 468, 82 }, 0.12f, 8, 2.0f + 2.0f * k,
                                    Fade(COR_DESTAQUE, 0.35f + 0.65f * k));
    }
    for (t = 0; t < NUM_ACOES; t++) {
        Rectangle r = { 20.0f + t * 92, 514, 88, 70 };
        bool disp = acaoDisponivel(b, jogador, (TipoAcao)t);
        bool sel = (ep->acao == (TipoAcao)t);
        Color cor = sel ? (Color){ 24, 24, 30, 255 } : (disp ? COR_TEXTO : (Color){ 100, 100, 112, 255 });
        if (botao(r, NULL, 20, true, sel)) {
            ep->acao = (TipoAcao)t;
            ep->escolheuAcao = true;
        }
        if (!disp && !sel) {
            DrawRectangleRounded(r, 0.2f, 6, Fade(BLACK, 0.45f));
        }
        snprintf(buf, sizeof(buf), "%d", t + 1);
        DrawText(buf, (int)r.x + 6, (int)r.y + 4, 10, cor);
        textoCentralizado(nomeAcao((TipoAcao)t), r.x + r.width / 2, r.y + 16, 20, cor);
        snprintf(buf, sizeof(buf), "E%d V%d", custoAcao((TipoAcao)t), velocidadeBase((TipoAcao)t));
        textoCentralizado(buf, r.x + r.width / 2, r.y + 40, 10, cor);
        if (danoAcao((TipoAcao)t) > 0) {
            snprintf(buf, sizeof(buf), "Dano %d", danoAcao((TipoAcao)t));
        } else {
            snprintf(buf, sizeof(buf), "%s", t == BLOQUEIO ? "Metade" : "Anula 1");
        }
        textoCentralizado(buf, r.x + r.width / 2, r.y + 54, 10, cor);
    }

    if (ehGolpe(ep->acao)) {
        DrawText("PUNHO (setas esq/dir)", 20, 594, 20, COR_DESTAQUE);
        for (i = 0; i < 2; i++) {
            Rectangle r = { 20.0f + i * 230, 620, 226, 40 };
            bool ativo = pecaAtiva(meu, punhoDoLado(i));
            if (botao(r, i == 0 ? "Punho esquerdo" : "Punho direito", 20, ativo, ep->lado == i)) {
                ep->lado = i;
            }
        }
        DrawText("Clique numa peça do rival com contorno", 20, 672, 20, COR_TEXTO_FRACO);
    } else {
        DrawText(ep->acao == BLOQUEIO ? "BLOQUEIO: não precisa de alvo"
                                      : "ESQUIVA: não precisa de alvo", 20, 594, 20, COR_DESTAQUE);
        if (botao((Rectangle){ 20, 620, 456, 40 }, "ADICIONAR (ESPAÇO)", 20,
                  acaoDisponivel(b, jogador, ep->acao), false)) {
            if (programarAcao(b, jogador, ep->acao, ep->lado, NUM_PECAS, ep->cursor, &motivo)) {
                ep->cursor++;
                ep->escolheuAcao = false;
            } else {
                avisar(ep, motivo);
            }
        }
    }

    /* coluna 2: a lista do jogador */
    n = tamanhoLista(*lista);
    DrawText("SUA LISTA (ordem = bônus de velocidade)", 492, 488, 20, COR_DESTAQUE);
    for (i = 0; i < MAX_ACOES_POR_JOGADOR; i++) {
        Rectangle s = retanguloSlot(i);
        Acao *a = acaoNaPosicao(*lista, i);
        bool sel = (i == ep->cursor);
        DrawRectangleRounded(s, 0.18f, 6, a ? COR_PAINEL_2 : COR_PAINEL);
        DrawRectangleRoundedLinesEx(s, 0.18f, 6, sel ? 3.0f : 1.0f, sel ? COR_DESTAQUE : COR_LINHA);
        snprintf(buf, sizeof(buf), "%dª", i + 1);
        DrawText(buf, (int)s.x + 12, (int)s.y + 8, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "+%d", bonusPosicao(i));
        DrawText(buf, (int)s.x + 12, (int)s.y + 32, 10, COR_TEXTO_FRACO);
        if (a != NULL) {
            descricaoAcao(a, buf, sizeof(buf));
            DrawText(buf, (int)s.x + 56, (int)s.y + 8, 20, COR_TEXTO);
            snprintf(buf, sizeof(buf), "velocidade %d    energia %d",
                     velocidadePrevista(b, jogador, a->tipo, i), custoAcao(a->tipo));
            DrawText(buf, (int)s.x + 56, (int)s.y + 34, 10, COR_TEXTO_FRACO);
            botao((Rectangle){ s.x + s.width - 124, s.y + 13, 36, 30 }, "^", 20, i > 0, false);
            botao((Rectangle){ s.x + s.width - 84, s.y + 13, 36, 30 }, "v", 20, i + 1 < n, false);
            botao((Rectangle){ s.x + s.width - 44, s.y + 13, 36, 30 }, "x", 20, true, false);
        } else if (sel) {
            DrawText("a nova ação entra aqui", (int)s.x + 56, (int)s.y + 18, 20, COR_TEXTO_FRACO);
        } else {
            DrawText("(vazio)", (int)s.x + 56, (int)s.y + 18, 20, COR_LINHA);
        }
    }
    if (n == MAX_ACOES_POR_JOGADOR) {
        DrawText("Lista cheia: remova (DEL) ou troque (SHIFT+setas)", 492, 700, 10, COR_TEXTO_FRACO);
    } else if (ep->cursor < n) {
        snprintf(buf, sizeof(buf), "A nova ação entra na posição %d e empurra as outras", ep->cursor + 1);
        DrawText(buf, 492, 700, 10, COR_TEXTO_FRACO);
    }

    /* coluna 3: energia e confirmação */
    {
        int energia = energiaRestante(b, jogador);
        DrawText("ENERGIA", 940, 488, 20, COR_DESTAQUE);
        for (i = 0; i < ENERGIA_POR_RODADA; i++) {
            Vector2 c = { 952.0f + i * 34, 534 };
            DrawCircleV(c, 13, i < energia ? COR_DESTAQUE : COR_PAINEL_2);
            DrawCircleLinesV(c, 13, COR_LINHA);
        }
        snprintf(buf, sizeof(buf), "%d/%d", energia, ENERGIA_POR_RODADA);
        DrawText(buf, 1126, 524, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "Velocidade do robô: +%d (pernas)", velocidadeDoRobo(meu));
        DrawText(buf, 940, 562, 10, COR_TEXTO_FRACO);
        DrawText("Setas cima/baixo: posição   SHIFT+setas: trocar", 940, 580, 10, COR_TEXTO_FRACO);
        DrawText("DEL: remover   F1: árvores e lista", 940, 596, 10, COR_TEXTO_FRACO);
        if (passo == PASSO_CONFIRMAR) {
            float k = pulso();
            DrawRectangleRoundedLinesEx((Rectangle){ 934, 614, 332, 68 }, 0.2f, 8, 2.0f + 2.0f * k,
                                        Fade(COR_DESTAQUE, 0.35f + 0.65f * k));
        }
        if (botao((Rectangle){ 940, 620, 320, 56 }, "CONFIRMAR (ENTER)", 20, n > 0, false)) {
            confirmar = true;
        }
    }

    /* aviso de erro */
    if (ep->tempoMensagem > 0.0f && ep->mensagem[0] != '\0') {
        int w = MeasureText(ep->mensagem, 20) + 40;
        Rectangle r = { (LARGURA_JANELA - w) / 2.0f, 92, (float)w, 36 };
        DrawRectangleRounded(r, 0.3f, 6, Fade((Color){ 80, 10, 10, 255 }, 0.9f));
        textoCentralizado(ep->mensagem, LARGURA_JANELA / 2.0f, 100, 20, COR_ERRO);
    }

    if (confirmar) {
        if (tamanhoLista(*lista) == 0) {
            avisar(ep, "Programe pelo menos 1 ação");
        } else {
            return TELA_TROCA;
        }
    }
    return TELA_PROGRAMACAO;
}

/* telaTroca: esconde tudo até o próximo jogador apertar ESPAÇO. */
Tela telaTroca(int jogador, int rodada)
{
    char buf[64];
    DrawRectangle(0, 0, LARGURA_JANELA, ALTURA_JANELA, (Color){ 10, 10, 14, 255 });
    snprintf(buf, sizeof(buf), "RODADA %d/%d", rodada, MAX_RODADAS);
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 200, 20, COR_TEXTO_FRACO);
    snprintf(buf, sizeof(buf), "Passe para o JOGADOR %d", jogador);
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 280, 60, corDoJogador(jogador));
    textoCentralizado("O outro jogador não deve olhar a tela.", LARGURA_JANELA / 2.0f, 380, 20, COR_TEXTO);
    textoCentralizado("ESPAÇO para continuar", LARGURA_JANELA / 2.0f, 470, 30, COR_DESTAQUE);
    if (tecla(KEY_SPACE)) {
        return TELA_PROGRAMACAO;
    }
    return TELA_TROCA;
}

/* ================= Resolução ================= */

#define CARTAO_L 158.0f
#define CARTAO_A 88.0f
#define FILA_X0  204.0f
#define FILA_Y   34.0f
#define FILA_PASSO 166.0f

/* reiniciarResolucao: começa mostrando a fila ordenada. */
void reiniciarResolucao(EstadoResolucao *er)
{
    memset(er, 0, sizeof(*er));
    er->fase = RES_MOSTRANDO_FILA;
}

/* acharCartao: cartão visual do nó (cria um novo se ainda não existe). */
static CartaoVisual *acharCartao(EstadoResolucao *er, const Acao *chave, float x, float y)
{
    int i, livre = -1;
    for (i = 0; i < MAX_CARTOES; i++) {
        if (er->cartoes[i].emUso && er->cartoes[i].chave == chave) {
            return &er->cartoes[i];
        }
        if (!er->cartoes[i].emUso && livre < 0) {
            livre = i;
        }
    }
    if (livre < 0) {
        return NULL;
    }
    er->cartoes[livre].emUso = true;
    er->cartoes[livre].chave = chave;
    er->cartoes[livre].x = x;
    er->cartoes[livre].y = y;
    er->cartoes[livre].velocidade = chave->velocidadeEfetiva;
    er->cartoes[livre].velocidadeAnterior = chave->velocidadeEfetiva;
    er->cartoes[livre].brilho = 0.0f;
    return &er->cartoes[livre];
}

/* desenharCartao: um cartão da fila (jogador, ação, velocidade). */
static void desenharCartao(Rectangle r, const Acao *a, int vel, int velAnterior, float brilho,
                           bool proximo, bool cancelado)
{
    char buf[64];
    Color cor = corDoJogador(a->jogador);
    Color fundo = escurecer(cor, 0.35f);
    DrawRectangleRounded(r, 0.15f, 6, fundo);
    if (brilho > 0.0f) {
        DrawRectangleRounded(r, 0.15f, 6, Fade(COR_DESTAQUE, 0.45f * brilho));
    }
    DrawRectangleRoundedLinesEx(r, 0.15f, 6, proximo ? 4.0f : 2.0f, proximo ? COR_DESTAQUE : cor);
    snprintf(buf, sizeof(buf), "J%d %s", a->jogador, nomeAcao(a->tipo));
    DrawText(buf, (int)r.x + 10, (int)r.y + 8, 20, COR_TEXTO);
    if (ehGolpe(a->tipo)) {
        snprintf(buf, sizeof(buf), "%s > %s", a->lado == FILHO_ESQ ? "Punho E" : "Punho D",
                 nomePeca(a->alvo));
    } else {
        snprintf(buf, sizeof(buf), "sem alvo");
    }
    DrawText(buf, (int)r.x + 10, (int)r.y + 34, 10, COR_TEXTO_FRACO);
    snprintf(buf, sizeof(buf), "vel %d", vel);
    DrawText(buf, (int)r.x + 10, (int)r.y + 56, 20, COR_DESTAQUE);
    if (brilho > 0.0f && velAnterior != vel) {
        snprintf(buf, sizeof(buf), "era %d", velAnterior);
        DrawText(buf, (int)(r.x + r.width - 10 - MeasureText(buf, 10)), (int)r.y + 62, 10, COR_TEXTO);
    }
    if (cancelado) {
        DrawRectangleRounded(r, 0.15f, 6, Fade(BLACK, 0.55f));
        DrawLineEx((Vector2){ r.x + 6, r.y + r.height - 8 }, (Vector2){ r.x + r.width - 6, r.y + 8 },
                   4.0f, COR_ERRO);
        textoCentralizado("CANCELADO", r.x + r.width / 2, r.y + r.height / 2 - 5, 10, COR_ERRO);
    }
}

/* atualizarCartoes: move cada cartão em direção ao seu lugar na fila. */
static void atualizarCartoes(Batalha *b, EstadoResolucao *er, float dt)
{
    const Acao *a;
    int i, indice = 0;
    float k = (dt * 7.0f > 1.0f) ? 1.0f : dt * 7.0f;
    for (i = 0; i < MAX_CARTOES; i++) {
        er->cartoes[i].emUso = er->cartoes[i].emUso && er->cartoes[i].chave != NULL;
    }
    /* marca quais nós ainda existem */
    {
        bool vivo[MAX_CARTOES] = { false };
        for (i = 0; i < MAX_CARTOES; i++) {
            for (a = b->fila; a != NULL; a = a->prox) {
                if (er->cartoes[i].chave == a) vivo[i] = true;
            }
            for (a = b->canceladas; a != NULL; a = a->prox) {
                if (er->cartoes[i].chave == a) vivo[i] = true;
            }
            if (!vivo[i]) {
                er->cartoes[i].emUso = false;
                er->cartoes[i].chave = NULL;
            }
        }
    }
    for (a = b->fila; a != NULL; a = a->prox, indice++) {
        float alvoX = FILA_X0 + indice * FILA_PASSO;
        CartaoVisual *c = acharCartao(er, a, alvoX + 260.0f, FILA_Y);
        if (c == NULL) continue;
        c->x += (alvoX - c->x) * k;
        c->y += (FILA_Y - c->y) * k;
        if (c->velocidade != a->velocidadeEfetiva) {
            c->velocidadeAnterior = c->velocidade;
            c->velocidade = a->velocidadeEfetiva;
            c->brilho = 1.0f;
        }
        if (c->brilho > 0.0f) {
            c->brilho -= dt * 0.6f;
        }
    }
    for (a = b->canceladas; a != NULL; a = a->prox, indice++) {
        float alvoX = FILA_X0 + indice * FILA_PASSO;
        CartaoVisual *c = acharCartao(er, a, alvoX, FILA_Y);
        if (c == NULL) continue;
        c->x += (alvoX - c->x) * k;
        c->y += (FILA_Y - c->y) * k;
    }
}

/* iniciarPasso: executa a próxima ação da fila (regra em batalha.c). */
static void iniciarPasso(Batalha *b, EstadoResolucao *er)
{
    executarProximaAcao(b, &er->evento);
    er->fase = RES_ANIMANDO;
    er->tempo = 0.0f;
}

/*
 * telaResolucao: mostra a fila ordenada e executa uma ação por vez, com
 * animação de ~1 s (ESPAÇO adianta). Devolve TELA_RESUMO quando a fila
 * acaba ou TELA_FIM quando um núcleo cai.
 */
Tela telaResolucao(Batalha *b, EstadoResolucao *er)
{
    float dt = entradaLiberada ? GetFrameTime() : 0.0f;   /* F1 pausa */
    bool pular = tecla(KEY_SPACE);
    Vector2 pos[2] = { { 400, RES_CHAO }, { 880, RES_CHAO } };
    AnimacaoRobo anim[2];
    const Evento *ev = &er->evento;
    char buf[96];
    float t;
    int i;
    const Acao *a;

    /* ---------- avanço do tempo ---------- */
    er->tempo += dt;
    switch (er->fase) {
    case RES_MOSTRANDO_FILA:
        if (er->tempo >= DURACAO_FILA || pular) {
            if (b->fila == NULL) {
                er->fase = RES_TERMINADA;
                er->tempo = 0.0f;
            } else {
                iniciarPasso(b, er);
            }
        }
        break;
    case RES_ANIMANDO:
        if (er->tempo >= DURACAO_ANIMACAO || pular) {
            if (b->vencedor != 0 || b->fila == NULL) {
                er->fase = RES_TERMINADA;
                er->tempo = 0.0f;
            } else {
                iniciarPasso(b, er);
            }
        }
        break;
    case RES_TERMINADA:
        if (er->tempo >= 1.6f || pular) {
            return (b->vencedor != 0) ? TELA_FIM : TELA_RESUMO;
        }
        break;
    }
    atualizarCartoes(b, er, dt);

    /* ---------- animação dos robôs ---------- */
    t = er->tempo / DURACAO_ANIMACAO;
    if (t > 1.0f) t = 1.0f;
    for (i = 0; i < 2; i++) {
        animacaoPadrao(&anim[i]);
        anim[i].escudo = b->bloqueioAtivo[i];
        anim[i].destacarNucleo = pecaExposta(b->robo[i], NUCLEO);
    }
    if (er->fase == RES_ANIMANDO && ev->executou) {
        int p = ev->acao.jogador - 1;
        int q = 1 - p;
        if (ehGolpe(ev->acao.tipo)) {
            float s = t / 0.55f;
            anim[p].ladoSoco = ev->acao.lado;
            anim[p].avancoSoco = 150.0f * sinf((s > 1.0f ? 1.0f : s) * PI_F);
            if (ev->esquivado) {
                float e = t / 0.6f;
                anim[q].recuo = 50.0f * sinf((e > 1.0f ? 1.0f : e) * PI_F);
            } else if (!ev->noVazio && t > 0.2f) {
                anim[q].pecaPiscando = ev->acao.alvo;
                anim[q].piscar = 1.0f - t;
            }
        } else if (ev->acao.tipo == ESQUIVA) {
            anim[p].recuo = 30.0f * sinf(t * PI_F);
        }
    }

    /* ---------- desenho ---------- */
    desenharFundoRingue(RES_CHAO);
    desenharRobo(b->robo[0], pos[0], RES_ESCALA, false, COR_J1, &anim[0]);
    desenharRobo(b->robo[1], pos[1], RES_ESCALA, true, COR_J2, &anim[1]);

    /* esquivas guardadas */
    for (i = 0; i < 2; i++) {
        if (b->esquivasPendentes[i] > 0) {
            snprintf(buf, sizeof(buf), "ESQUIVA x%d", b->esquivasPendentes[i]);
            textoCentralizado(buf, pos[i].x, RES_CHAO - 430, 20, SKYBLUE);
        }
    }

    /* texto flutuante do efeito */
    if (er->fase == RES_ANIMANDO && ev->executou) {
        int p = ev->acao.jogador - 1;
        int q = 1 - p;
        Vector2 ponto = { pos[p].x, RES_CHAO - 440 };
        Color cor = COR_TEXTO;
        buf[0] = '\0';
        if (ehGolpe(ev->acao.tipo)) {
            Rectangle r = caixaPeca(ev->acao.alvo, pos[q], RES_ESCALA, q == 1, NULL);
            ponto = (Vector2){ r.x + r.width / 2, r.y - 10 };
            if (ev->esquivado) {
                snprintf(buf, sizeof(buf), "ESQUIVOU!");
                cor = SKYBLUE;
            } else if (ev->noVazio) {
                snprintf(buf, sizeof(buf), "NO VAZIO");
                cor = COR_TEXTO_FRACO;
            } else {
                snprintf(buf, sizeof(buf), "-%d%s", ev->danoCausado, ev->bloqueado ? " BLOQUEADO" : "");
                cor = ev->pecaDestruida ? COR_ERRO : COR_DESTAQUE;
            }
        } else if (ev->acao.tipo == BLOQUEIO) {
            snprintf(buf, sizeof(buf), "GUARDA!");
            cor = SKYBLUE;
        } else {
            snprintf(buf, sizeof(buf), "ESQUIVA PRONTA");
            cor = SKYBLUE;
        }
        textoCentralizado(buf, ponto.x + 2, ponto.y - 50.0f * t + 2, 30, Fade(BLACK, 0.8f - 0.5f * t));
        textoCentralizado(buf, ponto.x, ponto.y - 50.0f * t, 30, Fade(cor, 1.0f - 0.6f * t));
    }

    /* fila de execução */
    DrawRectangle(0, 0, LARGURA_JANELA, 128, Fade(COR_FUNDO, 0.92f));
    DrawText("EXECUTANDO", 20, 10, 20, COR_DESTAQUE);
    DrawText("FILA DE EXECUÇÃO (ordenada por velocidade)", (int)FILA_X0, 10, 20, COR_TEXTO);
    {
        Rectangle caixa = { 20, FILA_Y, CARTAO_L, CARTAO_A };
        if (er->fase != RES_MOSTRANDO_FILA && ev->executou) {
            desenharCartao(caixa, &ev->acao, ev->acao.velocidadeEfetiva, ev->acao.velocidadeEfetiva,
                           0.0f, true, false);
        } else {
            DrawRectangleRoundedLinesEx(caixa, 0.15f, 6, 2.0f, COR_LINHA);
            textoCentralizado("-", caixa.x + caixa.width / 2, caixa.y + 34, 20, COR_LINHA);
        }
    }
    for (i = 0; i < MAX_CARTOES; i++) {
        CartaoVisual *c = &er->cartoes[i];
        bool cancelado = false;
        if (!c->emUso) continue;
        for (a = b->canceladas; a != NULL; a = a->prox) {
            if (a == c->chave) cancelado = true;
        }
        desenharCartao((Rectangle){ c->x, c->y, CARTAO_L, CARTAO_A }, c->chave, c->velocidade,
                       c->velocidadeAnterior, c->brilho, c->chave == b->fila && !cancelado, cancelado);
    }
    if (b->fila == NULL && b->canceladas == NULL) {
        DrawText("(fila vazia)", (int)FILA_X0, (int)FILA_Y + 34, 20, COR_LINHA);
    }

    /* barras e rodada */
    desenharBarraIntegridade((Rectangle){ 40, 138, 460, 26 }, 1, b->robo[0]);
    desenharBarraIntegridade((Rectangle){ 780, 138, 460, 26 }, 2, b->robo[1]);
    snprintf(buf, sizeof(buf), "RODADA %d/%d", b->rodada, MAX_RODADAS);
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 141, 20, COR_TEXTO);

    /* narração */
    if (er->fase == RES_MOSTRANDO_FILA) {
        snprintf(buf, sizeof(buf), "Listas intercaladas (J%d começou) e ordenadas. Lutem!",
                 (b->rodada % 2 == 1) ? 1 : 2);
        textoCentralizado(buf, LARGURA_JANELA / 2.0f, 182, 20, COR_TEXTO);
    } else if (ev->executou) {
        textoCentralizado(ev->texto, LARGURA_JANELA / 2.0f, 182, 20, COR_TEXTO);
        if (ev->canceladasNoPasso > 0) {
            textoCentralizado(ev->textoCancelamento, LARGURA_JANELA / 2.0f, 210, 20, COR_ERRO);
        }
    }
    if (er->fase == RES_TERMINADA) {
        if (b->vencedor != 0 && b->vitoriaPorNucleo) {
            textoCentralizado("K.O.!", LARGURA_JANELA / 2.0f + 4, 284, 100, Fade(BLACK, 0.6f));
            textoCentralizado("K.O.!", LARGURA_JANELA / 2.0f, 280, 100, COR_DESTAQUE);
            textoCentralizado("Núcleo destruído", LARGURA_JANELA / 2.0f, 390, 30, COR_TEXTO);
        } else {
            textoCentralizado("FIM DA RODADA", LARGURA_JANELA / 2.0f, 300, 40, COR_DESTAQUE);
        }
    }
    DrawText("ESPAÇO: adiantar    F1: depuração", 20, ALTURA_JANELA - 22, 10, COR_TEXTO_FRACO);
    return TELA_RESOLUCAO;
}

/* ================= Resumo e Fim ================= */

/* telaResumo: o que aconteceu na rodada e as integridades. */
Tela telaResumo(Batalha *b, int rodadaEncerrada)
{
    char buf[64];
    int i;
    DrawRectangle(0, 0, LARGURA_JANELA, ALTURA_JANELA, COR_FUNDO);
    snprintf(buf, sizeof(buf), "FIM DA RODADA %d/%d", rodadaEncerrada, MAX_RODADAS);
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 26, 40, COR_DESTAQUE);

    DrawRectangleRounded((Rectangle){ 40, 90, 700, 540 }, 0.04f, 6, COR_PAINEL);
    DrawText("O que aconteceu", 60, 104, 20, COR_DESTAQUE);
    for (i = 0; i < b->numRegistros && i < 18; i++) {
        const char *linha = b->registro[i];
        Color cor = (strstr(linha, "cancelado") != NULL) ? COR_ERRO
                  : (linha[1] == '1') ? (Color){ 255, 170, 170, 255 } : (Color){ 170, 200, 255, 255 };
        DrawText(linha, 60, 138 + i * 26, 20, cor);
    }
    if (b->numRegistros == 0) {
        DrawText("Nenhuma ação executada.", 60, 138, 20, COR_TEXTO_FRACO);
    }

    desenharRobo(b->robo[0], (Vector2){ 860, 560 }, 0.6f, false, COR_J1, NULL);
    desenharRobo(b->robo[1], (Vector2){ 1120, 560 }, 0.6f, true, COR_J2, NULL);
    desenharBarraIntegridade((Rectangle){ 770, 590, 470, 26 }, 1, b->robo[0]);
    desenharBarraIntegridade((Rectangle){ 770, 624, 470, 26 }, 2, b->robo[1]);

    textoCentralizado(b->vencedor != 0 ? "ESPAÇO: ver o resultado" : "ESPAÇO: próxima rodada",
                      LARGURA_JANELA / 2.0f, 670, 20, COR_TEXTO);
    if (tecla(KEY_SPACE) || tecla(KEY_ENTER)) {
        return (b->vencedor != 0) ? TELA_FIM : TELA_TROCA;
    }
    return TELA_RESUMO;
}

/* telaFim: vencedor, integridade final e placar. R revanche, M menu. */
Tela telaFim(Batalha *b, const Placar *placar)
{
    char buf[96];
    Color cor = (b->vencedor == 1) ? COR_J1 : (b->vencedor == 2) ? COR_J2 : COR_DESTAQUE;
    desenharFundoRingue(560.0f);
    DrawRectangle(0, 0, LARGURA_JANELA, 190, Fade(COR_FUNDO, 0.85f));
    if (b->vencedor == EMPATE) {
        snprintf(buf, sizeof(buf), "EMPATE");
    } else {
        snprintf(buf, sizeof(buf), "VITÓRIA DO JOGADOR %d", b->vencedor);
    }
    textoCentralizado(buf, LARGURA_JANELA / 2.0f + 3, 33, 60, Fade(BLACK, 0.6f));
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 30, 60, cor);
    if (b->vitoriaPorNucleo) {
        snprintf(buf, sizeof(buf), "Núcleo destruído na rodada %d", b->rodada);
    } else {
        snprintf(buf, sizeof(buf), "Fim das %d rodadas: vence a maior integridade", MAX_RODADAS);
    }
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 104, 20, COR_TEXTO);
    snprintf(buf, sizeof(buf), "Placar da sessão:  J1 %d x %d J2   (empates: %d)",
             placar->vitorias[0], placar->vitorias[1], placar->empates);
    textoCentralizado(buf, LARGURA_JANELA / 2.0f, 142, 20, COR_DESTAQUE);

    desenharRobo(b->robo[0], (Vector2){ 400, 560 }, 0.9f, false, COR_J1, NULL);
    desenharRobo(b->robo[1], (Vector2){ 880, 560 }, 0.9f, true, COR_J2, NULL);
    desenharBarraIntegridade((Rectangle){ 40, 600, 560, 30 }, 1, b->robo[0]);
    desenharBarraIntegridade((Rectangle){ 680, 600, 560, 30 }, 2, b->robo[1]);

    textoCentralizado("R: revanche        M: menu", LARGURA_JANELA / 2.0f, 660, 30, COR_TEXTO);
    if (tecla(KEY_R)) {
        return TELA_PROGRAMACAO;
    }
    if (tecla(KEY_M)) {
        return TELA_MENU;
    }
    return TELA_FIM;
}

/* ================= Depuração (F1) ================= */

/*
 * desenharNoArvore: desenha a árvore como diagrama, recursivamente em
 * pré-ordem: arestas para os filhos, a caixa do nó e depois os filhos.
 */
static void desenharNoArvore(const Peca *p, float x, float y, float dx)
{
    char buf[32];
    float filhoY = y + 84.0f;
    Rectangle r = { x - 52, y - 24, 104, 48 };
    if (p == NULL) {
        return;
    }
    if (p->esq != NULL) {
        DrawLineEx((Vector2){ x, y }, (Vector2){ x - dx, filhoY }, 2.0f, COR_TEXTO_FRACO);
    }
    if (p->dir != NULL) {
        DrawLineEx((Vector2){ x, y }, (Vector2){ x + dx, filhoY }, 2.0f, COR_TEXTO_FRACO);
    }
    DrawRectangleRounded(r, 0.2f, 6, COR_PAINEL_2);
    DrawRectangleRoundedLinesEx(r, 0.2f, 6, 3.0f, corVida(p));
    textoCentralizado(p->nome, x, y - 18, 20, p->ativa ? COR_TEXTO : COR_TEXTO_FRACO);
    snprintf(buf, sizeof(buf), "%d/%d%s", p->resistencia, p->resistenciaMax, p->ativa ? "" : " off");
    textoCentralizado(buf, x, y + 8, 10, corVida(p));
    desenharNoArvore(p->esq, x - dx, filhoY, dx / 2.0f);
    desenharNoArvore(p->dir, x + dx, filhoY, dx / 2.0f);
}

/* desenharSeta: seta horizontal de (x1,y) até (x2,y). */
static void desenharSeta(float x1, float x2, float y, Color cor)
{
    DrawLineEx((Vector2){ x1, y }, (Vector2){ x2, y }, 2.0f, cor);
    DrawLineEx((Vector2){ x2, y }, (Vector2){ x2 - 8, y - 6 }, 2.0f, cor);
    DrawLineEx((Vector2){ x2, y }, (Vector2){ x2 - 8, y + 6 }, 2.0f, cor);
}

/* desenharListaDiagrama: nós da lista encadeada ligados por setas até NULL. */
static void desenharListaDiagrama(const Acao *lista, float x, float y, const char *rotulo)
{
    char buf[64];
    DrawText(rotulo, (int)x, (int)y - 26, 20, COR_DESTAQUE);
    for (; lista != NULL; lista = lista->prox) {
        Rectangle r = { x, y, 168, 66 };
        DrawRectangleRounded(r, 0.15f, 6, COR_PAINEL_2);
        DrawRectangleRoundedLinesEx(r, 0.15f, 6, 2.0f, corDoJogador(lista->jogador));
        snprintf(buf, sizeof(buf), "J%d %s", lista->jogador, nomeAcao(lista->tipo));
        DrawText(buf, (int)x + 8, (int)y + 6, 20, COR_TEXTO);
        snprintf(buf, sizeof(buf), "vel %d  pos %d  ord %d", lista->velocidadeEfetiva,
                 lista->posicao, lista->ordemIntercalacao);
        DrawText(buf, (int)x + 8, (int)y + 32, 10, COR_TEXTO_FRACO);
        if (ehGolpe(lista->tipo)) {
            snprintf(buf, sizeof(buf), "%s > %s", lista->lado == FILHO_ESQ ? "Punho E" : "Punho D",
                     nomePeca(lista->alvo));
            DrawText(buf, (int)x + 8, (int)y + 48, 10, COR_TEXTO_FRACO);
        }
        desenharSeta(x + 168, x + 194, y + 33, COR_TEXTO);
        x += 198;
    }
    DrawText("NULL", (int)x, (int)y + 23, 20, COR_TEXTO_FRACO);
}

/*
 * desenharDepuracao: sobreposição do F1 com a árvore de cada robô
 * (nós, arestas e resistência) e a lista atual com setas.
 */
void desenharDepuracao(Batalha *b, const Acao *lista, const char *tituloLista,
                       const Acao *canceladas)
{
    DrawRectangle(0, 0, LARGURA_JANELA, ALTURA_JANELA, Fade((Color){ 8, 8, 12, 255 }, 0.94f));
    textoCentralizado("DEPURAÇÃO (F1 fecha)", LARGURA_JANELA / 2.0f, 12, 20, COR_DESTAQUE);
    DrawText("Robô J1 - árvore binária", 30, 44, 20, COR_J1);
    DrawText("Robô J2 - árvore binária", 680, 44, 20, COR_J2);
    DrawLine(LARGURA_JANELA / 2, 40, LARGURA_JANELA / 2, 400, COR_LINHA);
    desenharNoArvore(b->robo[0], 330, 104, 150);
    desenharNoArvore(b->robo[1], 955, 104, 150);

    if (tituloLista != NULL) {
        desenharListaDiagrama(lista, 20, 470, tituloLista);
    }
    if (canceladas != NULL) {
        desenharListaDiagrama(canceladas, 20, 600, "Canceladas nesta rodada");
    }
}
