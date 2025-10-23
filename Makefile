CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

SRC = main.c src/parser.c src/executor.c src/builtin.c src/files.c src/warp.c src/history.c src/input.c src/jobs.c src/builtin_jobs.c
OBJ = $(SRC:.c=.o)
TARGET = cshell

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean
