#include "atc.h"
#include <stdio.h>

// Variáveis globais (definidas em atc.c)
extern Controlador controlador;
extern Aeronave *aeronaves;
extern int N;
extern int executando;
extern struct timespec inicio_simulacao;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <num_setores> <num_aeronaves>\n", argv[0]);
        printf("Exemplo: %s 10 5\n", argv[0]);
        return 1;
    }
    
    int M = atoi(argv[1]); // Número de setores
    int N = atoi(argv[2]); // Número de aeronaves
    
    if (M <= 0 || N <= 0) {
        printf("Erro: ambos os números devem ser positivos\n");
        return 1;
    }
    
    printf("=== SISTEMA DE CONTROLE DE TRÁFEGO AÉREO ===\n");
    printf("Setores: %d | Aeronaves: %d\n\n", M, N);
    
    // Inicializar sistema
    inicializar_sistema(M, N);
    
    // Criar thread do controlador
    pthread_t thread_controlador_id;
    pthread_create(&thread_controlador_id, NULL, thread_controlador, NULL);
    
    // Criar threads das aeronaves
    for (int i = 0; i < N; i++) {
        pthread_create(&aeronaves[i].thread, NULL, thread_aeronave, &aeronaves[i]);
    }
    
    // Aguardar término das aeronaves
    for (int i = 0; i < N; i++) {
        pthread_join(aeronaves[i].thread, NULL);
    }
    
    // Finalizar
    executando = 0;
    pthread_join(thread_controlador_id, NULL);
    finalizar_sistema();
    
    return 0;
}