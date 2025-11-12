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

OUT = $(OUTDIR)/main$(EXE)

MKDIR_P = mkdir -p
RM = rm -f

.PHONY: all clean

all: $(OUT)

$(OUTDIR):
	@$(MKDIR_P)

$(OUT): $(OBJS) | $(OUTDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	-$(RM) $(OBJS) $(OUT)
