#include "setor.h"
#include <stdio.h>
#include <stdlib.h>

// Função para inicializar um setor
void inicializar_setor(Setor* setor, int id) {
    setor->id = id;
    setor->ocupado = 0;
    setor->aeronave_ocupante = -1;
    setor->proxima_aeronave = -1;
    
    if (pthread_mutex_init(&setor->mutex, NULL) != 0) {
        perror("Erro ao inicializar mutex do setor");
        exit(EXIT_FAILURE);
    }
}

// Função para destruir um setor
void destruir_setor(Setor* setor) {
    pthread_mutex_destroy(&setor->mutex);
}

// Função para tentar ocupar um setor
int tentar_ocupar_setor(Setor* setor, int aeronave_id) {
    pthread_mutex_lock(&setor->mutex);
    
    if (setor->ocupado) {
        pthread_mutex_unlock(&setor->mutex);
        return 0; // Setor já ocupado
    }
    
    setor->ocupado = 1;
    setor->aeronave_ocupante = aeronave_id;
    
    pthread_mutex_unlock(&setor->mutex);
    return 1; // Sucesso ao ocupar o setor
}

// Função para liberar um setor
void liberar_setor(Setor* setor) {
    pthread_mutex_lock(&setor->mutex);
    
    setor->ocupado = 0;
    setor->aeronave_ocupante = -1;
    
    pthread_mutex_unlock(&setor->mutex);
}