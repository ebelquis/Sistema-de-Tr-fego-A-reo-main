#ifndef CONTROLE_H
#define CONTROLE_H

#include "setor.h"
#include "aeronave.h"

typedef struct {
    Setor* setores;
    int num_setores;
    int num_aeronaves;
    int aeronaves_concluidas;
    pthread_mutex_t mutex_global;
} Controle;

// Protótipos das funções
Controle* inicializar_controle(int num_setores, int num_aeronaves);
void destruir_controle(Controle* controle);
void imprimir_estado_controle(Controle* controle);

#endif