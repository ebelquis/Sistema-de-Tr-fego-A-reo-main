#ifndef AERONAVE_H
#define AERONAVE_H

#include <pthread.h>
#include "setor.h"

#define MAX_ROTA 100 // Número máximo de setores na rota

typedef struct {
    int id; // Identificador da aeronave
    unsigned int prioridade; // de 0 (baixa) a 1000 (alta)
    int tamanho_rota; // Número de setores na rota
    int rota[MAX_ROTA]; // Rota da aeronave (array dos IDs dos Setores)
    int setor_atual; // ID do setor atual na rota
    int indice_rota_atual; // indice atual na rota
} Aeronave;

// Estrutura para passar argumentos para a thread da aeronave
typedef struct {
    int id;
    int num_setores_total;
} ArgsAeronave;

// Inicializa todas as aeronaves
void* rotina_aeronave(void* arg);
void inicializar_aeronaves(Aeronave *aeronaves, int N);

// Libera recursos das aeronaves
void destruir_aeronaves(Aeronave *aeronaves, int N);

#endif