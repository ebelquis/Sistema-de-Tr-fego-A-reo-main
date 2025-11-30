#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "aeronave.h"
#include "setor.h"

// Variáveis globais (simplificadas para começar)
int NUM_SETORES;
int NUM_AERONAVES;

// Função auxiliar para gerar um número aleatório entre min e max
int gerar_numero(int min, int max) {
    return min + rand() % (max - min + 1);
}

// A função que cada thread (aeronave) vai executar
void* rotina_aeronave(void* arg) {
    ArgsAeronave* args = (ArgsAeronave*) arg;
    
    // 1. Criar a aeronave e seus atributos 
    Aeronave minha_nave;
    minha_nave.id = args->id;
    minha_nave.prioridade = gerar_numero(1, 1000); // Prioridade aleatória 
    minha_nave.tamanho_rota = gerar_numero(2, MAX_ROTA); // Rota de tamanho variável 
    minha_nave.indice_rota_atual = 0;
    minha_nave.setor_atual = -1; // -1 indica que está no chão/fora do espaço aéreo

    // Gerar rota aleatória
    printf("[Aeronave %d] Prioridade: %d, Rota: ", minha_nave.id, minha_nave.prioridade);
    for(int i = 0; i < minha_nave.tamanho_rota; i++) {
        minha_nave.rota[i] = gerar_numero(0, args->num_setores_total - 1);
        printf("%d ", minha_nave.rota[i]);
    }
    printf("\n");

    // 2. Ciclo de vida da aeronave 
    // AQUI entra a lógica de voo.
    
    free(args); // Libera a memória do argumento
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // 1. Verificar argumentos 
    if (argc < 3) {
        printf("Uso: %s <num_setores> <num_aeronaves>\n", argv[0]);
        return 1;
    }

    NUM_SETORES = atoi(argv[1]);
    NUM_AERONAVES = atoi(argv[2]);
    
    srand(time(NULL)); // Inicializa aleatoriedade

    printf("Iniciando Simulação ATC: %d Setores, %d Aeronaves\n", NUM_SETORES, NUM_AERONAVES);

    // TODO: Inicializar a Torre de Controle (MIGUEL)
    
    // 2. Criar as threads das aeronaves 
    pthread_t threads[NUM_AERONAVES];
    
    for (int i = 0; i < NUM_AERONAVES; i++) {
        ArgsAeronave* args = malloc(sizeof(ArgsAeronave));
        args->id = i + 1;
        args->num_setores_total = NUM_SETORES;

        if (pthread_create(&threads[i], NULL, rotina_aeronave, (void*)args) != 0) {
            perror("Erro ao criar thread");
            return 1;
        }
    }

    // 3. Aguardar todas as aeronaves terminarem (pousarem)
    for (int i = 0; i < NUM_AERONAVES; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Simulação finalizada.\n");

    // TODO: Imprimir estatísticas finais (tempo médio) 
    
    return 0;
}