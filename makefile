# Makefile para o Sistema de Controle de Tráfego Aéreo

# Compilador a ser utilizado
CC = gcc

# Flags de compilação:
# -Wall: Ativa a maioria dos avisos de compilação (ajuda a evitar erros)
# -pthread: Necessário para usar a biblioteca de threads POSIX (requisito do enunciado)
# -g: Adiciona informações de depuração (útil se precisares usar o GDB)
CFLAGS = -Wall -pthread -g

# Lista de arquivos fonte (.c) do projeto
SRCS = main.c aeronave.c setor.c controle.c

# Lista de arquivos objeto (.o) gerados a partir dos fontes
# (Substitui automaticamente a extensão .c por .o na lista SRCS)
OBJS = $(SRCS:.c=.o)

# Nome do executável final
TARGET = atc_sim

# --- Regras do Makefile ---

# Regra padrão (executada quando rodas apenas 'make')
all: $(TARGET)

# Regra de Linkagem: Junta todos os arquivos objeto para criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Compilação concluída com sucesso! Executável: ./$(TARGET)"

# Regra genérica para compilar qualquer .c em .o
# $< representa o arquivo fonte (.c)
# $@ representa o arquivo alvo (.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra de limpeza: Remove os arquivos compilados para uma compilação limpa
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Arquivos temporários removidos."

# Regra para rodar o programa (exemplo de uso)
# Uso: make run ARGS="5 10" (para 5 setores e 10 aeronaves)
run: $(TARGET)
	./$(TARGET) $(ARGS)