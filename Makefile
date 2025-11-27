# Makefile básico: compila todos os .c no diretório e produz output/main(.exe em Windows)
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g3
CPPFLAGS = -I.
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

# Detecta Windows (cria executável com .exe) em ambientes onde MAKE definiu OS=Windows_NT
EXE :=
ifeq ($(OS),Windows_NT)
	EXE := .exe
endif

# Evitar chamar `uname` em Windows (pode não existir -> CreateProcess falha)
UNAME_S :=
ifeq ($(OS),Windows_NT)
	UNAME_S := Windows_NT
else
	UNAME_S := $(shell uname -s)
endif

# Detecta Linux (coloca o output como ./main) em ambientes linux
DOT :=
ifeq ($(UNAME_S),Linux)
	DOT := .
endif

# Saída sempre no diretório do projeto
OUT = ./main$(EXE)

MKDIR_P = mkdir -p
RM = rm -f

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	-$(RM) $(OBJS) $(OUT)
