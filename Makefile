CC = gcc
CFLAGS = -O1 -Wall -std=c99 -Wno-missing-braces -I include/
LDFLAGS = -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
SRC = main.c
SRC2 = stack.c
OUT = c-paint.exe

all:
	$(CC) $(SRC) $(SRC2) $(CFLAGS) $(LDFLAGS) -o $(OUT)