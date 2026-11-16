# Ringue de Aço - Makefile
#
#   make        compila o jogo em build/
#   make run    compila e abre o jogo
#   make test   compila e roda os testes da lógica (sem raylib)
#   make clean  apaga build/
#   make valgrind  roda os testes no valgrind (Linux)
#
# Onde está a raylib:
#   - Linux/macOS com pkg-config: detectada sozinha.
#   - Caso contrário passe a pasta que contém include/ e lib/:
#       make RAYLIB=C:/raylib
#     ou crie um arquivo local.mk com a linha  RAYLIB = C:/raylib

-include local.mk

CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
BUILD  = build

SRC_LOGICA = src/robo.c src/acoes.c src/batalha.c
SRC_JOGO   = src/main.c src/interface.c $(SRC_LOGICA)
HEADERS    = $(wildcard src/*.h)

ifeq ($(OS),Windows_NT)
    EXE          = .exe
    LIBS_SISTEMA = -lopengl32 -lgdi32 -lwinmm
    WRAP_MEMORIA = -DCONTAR_MEMORIA -Wl,--wrap=malloc -Wl,--wrap=free
else
    EXE =
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        LIBS_SISTEMA = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio
        WRAP_MEMORIA =
    else
        LIBS_SISTEMA = -lGL -lm -lpthread -ldl -lrt -lX11
        WRAP_MEMORIA = -DCONTAR_MEMORIA -Wl,--wrap=malloc -Wl,--wrap=free
    endif
endif

ifdef RAYLIB
    RAYLIB_CFLAGS = -I$(RAYLIB)/include
    RAYLIB_LIBS   = -L$(RAYLIB)/lib -lraylib $(LIBS_SISTEMA)
else
    TEM_PKG := $(shell pkg-config --exists raylib 2>/dev/null && echo sim)
    ifeq ($(TEM_PKG),sim)
        RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib)
        RAYLIB_LIBS   = $(shell pkg-config --libs raylib) $(LIBS_SISTEMA)
    else
        RAYLIB_LIBS   = -lraylib $(LIBS_SISTEMA)
    endif
endif

JOGO  = $(BUILD)/ringue-de-aco$(EXE)
TESTE = $(BUILD)/test_logica$(EXE)

.PHONY: all run test clean valgrind

all: $(JOGO)

$(BUILD):
	mkdir -p $(BUILD)

$(JOGO): $(SRC_JOGO) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(SRC_JOGO) -o $@ $(RAYLIB_LIBS)

$(TESTE): tests/test_logica.c $(SRC_LOGICA) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -O0 -g tests/test_logica.c $(SRC_LOGICA) -o $@ $(WRAP_MEMORIA)

run: $(JOGO)
	./$(JOGO)

test: $(TESTE)
	./$(TESTE)

valgrind: $(TESTE)
	valgrind --leak-check=full --error-exitcode=1 ./$(TESTE)

clean:
	rm -rf $(BUILD)
