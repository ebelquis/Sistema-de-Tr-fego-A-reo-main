#ifndef SETOR_H
#define SETOR_H

#include <pthread.h>

typedef struct {
    int id;
    pthread_mutex_t mutex;
    int ocupado; // 0 = livre, 1 = ocupado
    int aeronave_ocupante; // ID da aeronave que está no setor
} Setor;

// Inicializa todos os setores
void inicializar_setores(Setor *setores, int M);

// Libera recursos dos setores
void destruir_setores(Setor *setores, int M);

#endif
