#ifndef AERONAVE_H
#define AERONAVE_H

#include <time.h>

#define MAX_ROTA 100 // Número máximo de setores na rota

typedef struct {
    int id;
    unsigned int prioridade;
    int rota[MAX_ROTA];
    int tamanho_rota;
    int setor_atual;
    int indice_rota;
    double tempo_total_espera;
    struct timespec inicio_espera;
} Aeronave;

// Estrutura para passar argumentos para a thread da aeronave
typedef struct {
    int id;
    int num_setores_total;
} ArgsAeronave;

// Protótipos das funções
Aeronave* criar_aeronave(int id, int num_setores); // Cria uma aeronave
void destruir_aeronave(Aeronave* aeronave); // Libera memória da aeronave
void imprimir_aeronave(Aeronave* aeronave); // Imprime detalhes da aeronave

#endif