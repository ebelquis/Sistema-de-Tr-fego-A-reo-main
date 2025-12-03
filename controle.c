#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "setor.h"
#include "aeronave.h"
#include "controle.h"

Controle* inicializar_controle(int num_setores_, int num_aeronaves_) {
    Controle *ctrl = (Controle*)malloc(sizeof(Controle));

    if (!ctrl) {
        perror("Erro ao alocar memória para Controle");
        return NULL;
    }

    ctrl->num_setores = num_setores_;
    ctrl->num_aeronaves = num_aeronaves_;
    ctrl->aeronaves_concluidas = 0;
    
    // Inicializa setores
    ctrl->setores = (Setor*)malloc(num_setores_ * sizeof(Setor));

    if (!ctrl->setores) {
        perror("Erro ao alocar memória para Setores");
        free(ctrl);
        return NULL;
    }
    
    for (int i = 0; i < num_setores_; i++) {
        inicializar_setor(&ctrl->setores[i], i);
    }
    
    pthread_mutex_init(&ctrl->mutex_global, NULL);

    printf("Torre de Controle inicializada com %d setores e %d aeronaves.\n", num_setores_, num_aeronaves_);
    
    return ctrl;
}

void destruir_controle(Controle *controle) {
    if (!controle) return;

    // Imprime estado final antes de destruir
    imprimir_estado_controle(controle);

    // Destruir o mutex global
    pthread_mutex_destroy(&controle->mutex_global);

    // Destruir setores
    if (controle->setores) {
        for (int i = 0; i < controle->num_setores; i++) {
            destruir_setor(&controle->setores[i]);
        }
        free(controle->setores);
    }

    free(controle);
    printf("Torre de Controle destruída e recursos liberados.\n");
}

void imprimir_estado_controle(Controle* controle) {
    if (!controle) {
        printf("Controle não inicializado.\n");
        return;
    }

    printf("\n=== Estado Atual da Torre de Controle ===\n");
    printf("Número de Setores: %d\n", controle->num_setores);
    printf("Número de Aeronaves: %d\n", controle->num_aeronaves);
    printf("Aeronaves Concluídas: %d\n", controle->aeronaves_concluidas);

    // Calcular estatísticas dos setores 
    int setores_ocupados = 0;
    int setores_livres = 0;
    int total_aeronaves_em_setores = 0;

    printf("\n--- Estados dos Setores ---\n");
    for (int i = 0; i < controle->num_setores; i++) {
        Setor* setor = &controle->setores[i];
        
        // Tentar travar o mutex para ler o estado
        int pode_ler = pthread_mutex_trylock(&setor->mutex);

        if (pode_ler == 0) {
            if (setor->ocupado) {
                setores_ocupados++;
                total_aeronaves_em_setores++;
                printf("Setor %d: OCUPADO pela Aeronave %d\n", setor->id, setor->aeronave_ocupante);

                if (setor->proxima_aeronave != -1) {
                    printf("    Próxima Aeronave na fila: %d\n", setor->proxima_aeronave);
                }
                printf("\n");
            } else {
                setores_livres++;
                printf("Setor %d: LIVRE\n\n", setor->id);

                if (setor->proxima_aeronave != -1) {
                    printf("    Próxima Aeronave na fila: %d\n", setor->proxima_aeronave);
                }
                printf("\n");
            }
            pthread_mutex_unlock(&setor->mutex);
        } else {
            // Não conseguiu travar, setor possivelmente em uso
            printf("Setor %d: Estado desconhecido (em uso)\n\n", setor->id);
        }
    }

    printf("\n--- Estatísticas dos Setores ---\n");
    printf("Setores Ocupados: %d/%d (%.1f%%) \n", 
           setores_ocupados, controle->num_setores, 
           (setores_ocupados * 100.0) / controle->num_setores);
    printf("Setores Livres: %d/%d (%.1f%%) \n", 
           setores_livres, controle->num_setores,
           (setores_livres * 100.0) / controle->num_setores);
    printf("Total de Aeronaves em Setores: %d\n", total_aeronaves_em_setores);

    int aeronaves_em_voo = controle->num_aeronaves - total_aeronaves_em_setores - controle->aeronaves_concluidas;
    printf("Aeronaves em Voo: %d\n", aeronaves_em_voo);
    printf("==========================================\n\n");
}