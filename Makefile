CC = gcc -g 
SRCS = main.c log.c filter.c
FILES = $(addprefix src/, $(SRCS))
OBJS = $(FILES:.c=.o)
LIBS = $(shell sdl2-config --libs) -ldl -lm
CFLAGS = $(shell sdl2-config --cflags) 

%.o: %.c $(FILES)
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC 

build: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o ./../build/av-viewer

dist: build
		rm $(OBJS)

clean:
		rm $(OBJS)
