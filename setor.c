#include "setor.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "controle.h"
#include "aeronave.h"

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
int solicitar_setor(Controle* torre_controle, Aeronave* nave, int setor_destino) {
    char mensagem[100];
    //
    // Inicia contagem do tempo de espera
    struct timespec inicio;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    sprintf(mensagem, "Aeronave %d (prio %u) solicitou setor %d", 
            nave->id, nave->prioridade, setor_destino);
    print_log(mensagem);
    
    Setor* setor = &torre_controle->setores[setor_destino];
    
    // Tenta adquirir o setor
    pthread_mutex_lock(&setor->mutex);
    
    // Se o setor está ocupado, espera
    while (setor->ocupado) {
        sprintf(mensagem, "Aeronave %d (prio %u) AGUARDANDO setor %d (ocupado por aeronave %d)", 
                nave->id, nave->prioridade, setor_destino, setor->aeronave_ocupante);
        print_log(mensagem);

        // Adiciona esta aeronave como próxima na fila, se não estiver já
        if (setor->proxima_aeronave == -1) {
            setor->proxima_aeronave = nave->id;
        }
        
        // Libera o mutex temporariamente para evitar deadlock
        pthread_mutex_unlock(&setor->mutex);
        usleep(100000 + (rand() % 200000)); // Espera 100-300ms (aleatório)
        pthread_mutex_lock(&setor->mutex);
    }
    
    // Adquire o setor
    setor->ocupado = 1;
    setor->aeronave_ocupante = nave->id;
    
    // Calcula tempo de espera
    struct timespec fim;
    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo_espera = (fim.tv_sec - inicio.tv_sec) + 
                         (fim.tv_nsec - inicio.tv_nsec) / 1e9;
    nave->tempo_total_espera += tempo_espera;
    
    sprintf(mensagem, "Aeronave %d (prio %u) ACESSOU setor %d (espera: %.3fs)", 
            nave->id, nave->prioridade, setor_destino, tempo_espera);
    print_log(mensagem);
    
    // Se esta aeronave era a que estava na fila de espera, remove-a
    if (setor->proxima_aeronave == nave->id) {
        setor->proxima_aeronave = -1;
    }
    
    pthread_mutex_unlock(&setor->mutex);
    return 1;
}

// Função para liberar setor
void liberar_setor(Controle* torre_controle, int setor_id, int aeronave_id) {
    if (setor_id == -1) return;
    
    Setor* setor = &torre_controle->setores[setor_id];
    pthread_mutex_lock(&setor->mutex);
    
    char mensagem[100];
    sprintf(mensagem, "Setor %d liberado pela aeronave %d", setor_id, aeronave_id);
    print_log(mensagem);
    
    setor->ocupado = 0;
    setor->aeronave_ocupante = -1;
    
    pthread_mutex_unlock(&setor->mutex);
}