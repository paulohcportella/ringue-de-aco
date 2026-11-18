#!/usr/bin/env bash
# gravar_video.sh
# Grava um vídeo do jogo de verdade numa tela virtual (Xvfb), jogando uma
# luta completa com teclado e mouse simulados (xdotool).
# O binário ./gravacao é o jogo normal ligado com ferramentas/gravador.c
# (-Wl,--wrap=EndDrawing), que manda cada quadro para o ffmpeg.
# Uso (Linux): bash ferramentas/gravar_video.sh   -> gera video/ringue-de-aco.mp4
set -euo pipefail

export DISPLAY=:99
mkdir -p video
Xvfb :99 -screen 0 1280x720x24 &
sleep 2

# coordenadas do meio de cada peça na tela de programação
eval "$(./centros)"

# ---------- auxiliares ----------
tecla() {   # tecla <nome> [espera]
    xdotool keydown "$1"; sleep 0.08; xdotool keyup "$1"; sleep "${2:-0.7}"
}
clicar() {  # clicar <x> <y> [espera]
    xdotool mousemove "$1" "$2"; sleep 0.9
    xdotool mousedown 1; sleep 0.08; xdotool mouseup 1; sleep "${3:-0.8}"
}
descansar() { xdotool mousemove 1270 470; }

export GRAVAR_PIPE="ffmpeg -loglevel error -y -f rawvideo -pix_fmt rgba -s 1280x720 -r 30 -i - \
-c:v libx264 -preset veryfast -crf 22 -pix_fmt yuv420p video/ringue-de-aco.mp4"
descansar
./gravacao &
JOGO=$!
xdotool search --sync --name "Ringue" > /dev/null
sleep 1.5
sleep 3                                   # menu

# ---------- Como jogar ----------
tecla Down; tecla Return 7                # tela de regras
tecla Return 1.5                          # volta ao menu
tecla Up; tecla Return 2.5                # Nova luta

# ---------- Rodada 1 (a mesma da apresentação) ----------
tecla 3; tecla Right                      # J1: Gancho com o punho direito
clicar "$J2_TRONCO_X" "$J2_TRONCO_Y"
tecla 1; tecla Left                       # J1: Jab com o punho esquerdo
xdotool mousemove "$J2_BRACO_D_X" "$J2_BRACO_D_Y"; sleep 1.5
clicar "$J2_BRACO_D_X" "$J2_BRACO_D_Y" 2
descansar; tecla Return 2.5               # confirma; tela de troca
tecla space 1.5
tecla 4; tecla space                      # J2: Bloqueio
tecla 1; tecla Left                       # J2: Jab no Braço D do J1
clicar "$J1_BRACO_D_X" "$J1_BRACO_D_Y" 1.5
descansar; tecla Return 3.6               # resolução: fila na tela, 2 ações
tecla F1 6                                # F1: árvores e fila ao vivo
tecla F1 6.5                              # volta e termina a rodada
tecla space 1.5                           # resumo -> troca
tecla space 1.5

# ---------- Rodada 2: J1 derruba a Base do J2 ----------
tecla 3; tecla Right
clicar "$J2_BASE_X" "$J2_BASE_Y"
tecla 2; tecla Left
clicar "$J2_BASE_X" "$J2_BASE_Y" 1.5
descansar; tecla Return 2
tecla space 1.5
tecla 4; tecla space                      # J2: Bloqueio + Direto na Perna E do J1
tecla 2; tecla Left
clicar "$J1_PERNA_E_X" "$J1_PERNA_E_Y" 1.5
descansar; tecla Return 9.5               # resolução completa
tecla space 1.5
tecla space 1.5

# ---------- Rodada 3: Núcleo exposto, golpe final ----------
tecla 3; tecla Right
clicar "$J2_NUCLEO_X" "$J2_NUCLEO_Y"
tecla 2; tecla Left
clicar "$J2_NUCLEO_X" "$J2_NUCLEO_Y" 1.5
descansar; tecla Return 2
tecla space 1.5
tecla 4; tecla space                      # J2: Bloqueio + Jab que derruba a Perna E do J1
tecla 1; tecla Left
clicar "$J1_PERNA_E_X" "$J1_PERNA_E_Y" 1.5
descansar; tecla Return 10                # resolução até o Núcleo cair
sleep 5                                   # tela de vitória
tecla m 2                                 # volta ao menu
tecla Down; tecla Down; tecla Return 1    # Sair: o jogo fecha e o ffmpeg termina o arquivo

wait "$JOGO" || true
while pgrep -x ffmpeg > /dev/null; do sleep 1; done
ls -la video
