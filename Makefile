# Makefile básico: compila todos os .c no diretório e produz output/main.exe
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g3
CPPFLAGS = -I.
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
OUTDIR = output
OUT = $(OUTDIR)/main.exe

MKDIR_P = mkdir -p
RM = rm -f

.PHONY: all clean

all: $(OUT)

$(OUTDIR):
	@$(MKDIR_P) $(OUTDIR)

$(OUT): $(OBJS) | $(OUTDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	-$(RM) $(OBJS) $(OUT)
