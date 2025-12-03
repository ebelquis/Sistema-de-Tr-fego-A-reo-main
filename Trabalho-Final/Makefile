CC = gcc
CFLAGS = -Wall -pthread -O2
TARGET = atc_simulator
SRC = main.c atc.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c atc.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET) 8 6

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET) 5 3

.PHONY: all clean run valgrind