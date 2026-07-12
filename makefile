CC = gcc
CFLAGS = -Wall -Wextra -I./include

TARGET = tetris_test

# Please edit this if you want to verify garbage clearing logic
SRCS = src/libtetrisbrain/tetrisbrain.c src/tetrisu/tetrisu.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

.PHONY: all run clean re