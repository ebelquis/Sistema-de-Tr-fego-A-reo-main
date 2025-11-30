CC = gcc
CFLAGS = -Wall -pthread -g
# -Wall: Mostra todos os avisos
# -pthread: Inclui a biblioteca de threads 
# -g: Permite depuração (debugging)

all: atc_sim

atc_sim: main.c
	$(CC) $(CFLAGS) -o atc_sim main.c

clean:
	rm -f atc_sim