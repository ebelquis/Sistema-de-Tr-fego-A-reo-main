#ifndef SETOR_H
#define SETOR_H

#include <pthread.h>

typedef struct {
    int id;
    pthread_mutex_t mutex;
    int ocupado; // 0 = livre, 1 = ocupado
    int aeronave_ocupante; // ID da aeronave que está no setor
    int proxima_aeronave; // ID da próxima aeronave na fila (prioridade)
} Setor;

// Protótipos das funções
void inicializar_setor(Setor* setor, int id); // Inicializa um setor
void destruir_setor(Setor* setor); // Libera recursos do setor
int tentar_ocupar_setor(Setor* setor, int aeronave_id); // Tenta ocupar o setor
void liberar_setor(Setor* setor); // Libera o setor

#endif
