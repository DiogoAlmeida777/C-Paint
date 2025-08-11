CC = gcc
CFLAGS = -O1 -Wall -std=c99 -Wno-missing-braces -I include/
LDFLAGS = -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -lshell32
SRC = main.c
SRC2 = doublylinkedlist.c
SRC3 = OS_paths.c
OUT = c-paint.exe

all:
	$(CC) $(SRC) $(SRC2) $(SRC3) $(CFLAGS) $(LDFLAGS) -o $(OUT)