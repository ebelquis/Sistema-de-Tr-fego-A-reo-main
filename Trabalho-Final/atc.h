#ifndef ATC_H
#define ATC_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

// Estruturas de dados
typedef struct {
    int id;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int ocupado;
    int aeronave_atual;
} Setor;

typedef struct {
    int id;
    unsigned int prioridade;
    int *rota;
    int rota_tam;
    int setor_atual;
    int setor_destino;
    time_t tempo_inicio;
    double tempo_espera_total;
    pthread_t thread;
    pthread_cond_t cond;
    struct Aeronave *prox;
} Aeronave;

typedef struct {
    Setor *setores;
    Aeronave **fila_espera;
    int *tamanho_fila;
    int M;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Controlador;

// Funções principais
void inicializar_sistema(int M, int N);
void *thread_aeronave(void *arg);
void *thread_controlador(void *arg);
void solicitar_acesso(Aeronave *av);
void liberar_setor(Aeronave *av);
void finalizar_sistema();
void imprimir_estado(Aeronave *av, const char *acao);

#endif