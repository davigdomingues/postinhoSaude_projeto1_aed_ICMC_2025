# Makefile básico: compila todos os .c no diretório e produz output/main.exe
CC = gcc
CFLAGS = -Wall -Wextra -g3
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
OUTDIR = output
OUT = $(OUTDIR)/main.exe

all: $(OUT)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

$(OUT): $(OBJS) | $(OUTDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJS) $(OUT)
