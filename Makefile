CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99 -O2
LDFLAGS=-lSDL2 -lSDL2_net -lSDL2_mixer -lSDL2_ttf -lSDL2_gfx -lSDL2_image
SRC=$(wildcard src/*.c)
OBJS=$(SRC:src/%.c=obj/%.o)

.PHONY: all objs clean clean-settings

all: objs kaaris-vs-booba

objs:
	mkdir -p obj

kaaris-vs-booba: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -r obj
	rm kaaris-vs-booba

clean-settings:
	rm *.ini *.bin
