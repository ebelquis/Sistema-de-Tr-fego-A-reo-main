#include "controle.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

Controle* inicializar_controle(int M, int N) {
    Controle *ctrl = (Controle*)malloc(sizeof(Controle));
    ctrl->M = M; // Número de setores
    ctrl->N = N; // Número de aeronaves
    
    // Inicializa setores
    ctrl->setores = (Setor*)malloc(M * sizeof(Setor));
    inicializar_setores(ctrl->setores, M);
    
    // Inicializa aeronaves
    ctrl->aeronaves = (Aeronave**)malloc(N * sizeof(Aeronave*));
    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        ctrl->aeronaves[i] = criar_aeronave(i, M);
    }
    
    pthread_mutex_init(&ctrl->mutex_global, NULL);
    
    return ctrl;
}

void destruir_controle(Controle *controle) {
    if (controle) {
        for (int i = 0; i < controle->N; i++) {
            destruir_aeronave(controle->aeronaves[i]);
        }
        free(controle->aeronaves);
        destruir_setores(controle->setores, controle->M);
        free(controle->setores);
        pthread_mutex_destroy(&controle->mutex_global);
        free(controle);
    }
}