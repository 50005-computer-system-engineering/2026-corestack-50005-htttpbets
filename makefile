CC = gcc
CFLAGS = -Wall -Wextra -I./include

# LDFLAGS (Linker Flags) is where we tell gcc to link external libraries
LDFLAGS = -lncurses

TARGET = tetris_test

SRCS = src/libtetrisbrain/tetrisbrain.c src/tetrisu/tetrisu.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

.PHONY: all run clean re