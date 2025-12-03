#include "aeronave.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

Aeronave* criar_aeronave(int id, int num_setores) {
    Aeronave* aeronave = (Aeronave*)malloc(sizeof(Aeronave));
    if (!aeronave) {
        perror("Falha ao alocar memória para aeronave");
        exit(EXIT_FAILURE);
    }
    
    aeronave->id = id;
    aeronave->prioridade = rand() % 10; // Prioridade aleatória entre 0 e 9
    aeronave->tamanho_rota = (rand() % (num_setores - 1)) + 1; // Tamanho da rota entre 1 e num_setores-1
    aeronave->setor_atual = -1; // Ainda não entrou em nenhum setor
    aeronave->indice_rota = 0;
    aeronave->tempo_total_espera = 0.0;
    
    // Gera uma rota aleatória
    for (int i = 0; i < aeronave->tamanho_rota; i++) {
        int setor;
        do {
            setor = rand() % num_setores;
        } while (i > 0 && setor == aeronave->rota[i - 1]); // Evita setores consecutivos iguais
        aeronave->rota[i] = setor;
    }
    
    return aeronave;
} // Cria uma aeronave
void destruir_aeronave(Aeronave* aeronave) { // Libera memória da aeronave
    if (aeronave) {
        free(aeronave);
    }
}
void imprimir_aeronave(Aeronave* aeronave) { // Imprime detalhes da aeronave
    if (aeronave) {
        printf("Aeronave ID: %d\n", aeronave->id);
        printf("Prioridade: %u\n", aeronave->prioridade);
        printf("Rota: ");
        for (int i = 0; i < aeronave->tamanho_rota; i++) {
            printf("%d ", aeronave->rota[i]);
        }
        printf("\nTamanho da Rota: %d\n", aeronave->tamanho_rota);
        printf("Setor Atual: %d\n", aeronave->setor_atual);
        printf("Índice da Rota: %d\n", aeronave->indice_rota);
        printf("Tempo Total de Espera: %.2f segundos\n", aeronave->tempo_total_espera);
    }
}
