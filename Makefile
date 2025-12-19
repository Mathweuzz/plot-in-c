# TatuPlot - Makefile (C99 + SDL2)
# Uso:
#   make
#   make run ARGS='--expr "\\sin(x)"'
#   make clean

CC      := gcc
CSTD    := -std=c99
CFLAGS  := $(CSTD) -Wall -Wextra -pedantic -O2 -Iinclude
LDFLAGS :=

# SDL2 flags (prefer sdl2-config; fallback pkg-config)
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(strip $(SDL_CFLAGS)),)
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null)
endif

ifeq ($(strip $(SDL_CFLAGS)),)
$(warning Nao foi possivel obter flags do SDL2. Instale 'sdl2' e verifique sdl2-config ou pkg-config.)
endif

BIN_DIR   := bin
BUILD_DIR := build
TARGET    := $(BIN_DIR)/tatuplot

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(SDL_LIBS) -lm

run: all
	./$(TARGET) $(ARGS)

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
