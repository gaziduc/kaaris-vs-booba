CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99
LDFLAGS=-lSDL2 -lSDL2_net -lSDL2_mixer -lSDL2_ttf -lSDL2_gfx -lSDL2_image
SRC=data.c editor.c event.c file.c game.c list.c key.c main.c multi.c net.c option.c text.c transition.c
OBJS=$(SRC:.c=.o)

.PHONY: all clean

all: kaaris-vs-booba

kaaris-vs-booba: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm *.o kaaris-vs-booba
