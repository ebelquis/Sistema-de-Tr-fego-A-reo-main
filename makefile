CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_GNU_SOURCE
TARGET = atc_simulator
OBJS = main.o aeronave.o setor.o controle.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c aeronave.h setor.h controle.h
	$(CC) $(CFLAGS) -c main.c

aeronave.o: aeronave.c aeronave.h
	$(CC) $(CFLAGS) -c aeronave.c

setor.o: setor.c setor.h controle.h aeronave.h
	$(CC) $(CFLAGS) -c setor.c

controle.o: controle.c controle.h setor.h aeronave.h
	$(CC) $(CFLAGS) -c controle.c

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET) 5 10

.PHONY: all clean run