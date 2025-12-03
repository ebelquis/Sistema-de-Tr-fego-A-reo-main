#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "aeronave.h"
#include "setor.h"
#include "controle.h"

// Variáveis globais (simplificadas para começar)
int NUM_SETORES;
int NUM_AERONAVES;
Controle* torre_controle;
// Mutexes
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

// Função para obter timestamp atual
void get_timestamp(char* buffer) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* tm_info = localtime(&ts.tv_sec);
    strftime(buffer, 20, "%H:%M:%S", tm_info);
    sprintf(buffer + 8, ".%03ld", ts.tv_nsec / 1000000);
}

// Print log com timestamp
void print_log(const char* mensagem) {
    char timestamp[20];
    get_timestamp(timestamp);
    
    pthread_mutex_lock(&mutex_log);
    printf("[%s] %s\n", timestamp, mensagem);
    pthread_mutex_unlock(&mutex_log);
}

// Função auxiliar para gerar um número aleatório entre min e max
int gerar_numero(int min, int max) {
    return min + rand() % (max - min + 1);
}

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

// A função que cada thread (aeronave) vai executar
void* rotina_aeronave(void* arg) {
    ArgsAeronave* args = (ArgsAeronave*) arg;
    
    // 1. Criar a aeronave e seus atributos 
    Aeronave* minha_nave = criar_aeronave(args->id, NUM_SETORES);

    char mensagem[200];
    sprintf(mensagem, "Aeronave %d iniciou: prio=%u, rota=[", 
            minha_nave->id, minha_nave->prioridade);
    
    for(int i = 0; i < minha_nave->tamanho_rota; i++) {
        char num[10];
        sprintf(num, "%d%s", minha_nave->rota[i], 
                (i < minha_nave->tamanho_rota-1) ? "," : "");
        strcat(mensagem, num);
    }
    strcat(mensagem, "]");
    print_log(mensagem);

    // 2. Ciclo de vida da aeronave 

    while (minha_nave->indice_rota < minha_nave->tamanho_rota) {
        int setor_destino = minha_nave->rota[minha_nave->indice_rota];

        // Se não for o primeiro, libera o setor anterior
        if (minha_nave->setor_atual != -1) {
            liberar_setor(torre_controle, minha_nave->setor_atual, minha_nave->id);
        }

        // Tenta solicitar o setor
        solicitar_setor(torre_controle, minha_nave, setor_destino);
        
        // Atualiza o setor atual
        minha_nave->setor_atual = setor_destino;
        minha_nave->indice_rota++;

        // Simula o tempo de voo no setor
        int tempo_voo = gerar_numero(1, 4); // 1 a 3 segundos
        sleep(tempo_voo);

        sprintf(mensagem, "Aeronave %d completou voo no setor %d (%d segundos)", 
                minha_nave->id, setor_destino, tempo_voo);
        print_log(mensagem);
    }

    // Libera último setor ao final da rota
    if (minha_nave->setor_atual != -1) {
        liberar_setor(torre_controle, minha_nave->setor_atual, minha_nave->id);
    }
    
    sprintf(mensagem, "Aeronave %d completou sua rota. Tempo total de espera: %.3fs", 
            minha_nave->id, minha_nave->tempo_total_espera);
    print_log(mensagem);
    
    // Registra conclusão na torre de controle
    pthread_mutex_lock(&torre_controle->mutex_global);
    torre_controle->aeronaves_concluidas++;
    pthread_mutex_unlock(&torre_controle->mutex_global);
    
    // Libera memória
    destruir_aeronave(minha_nave);
    free(args);
    
    pthread_exit(NULL);
}

void imprimir_estatisticas() {
    printf("\n=== ESTATÍSTICAS DA SIMULAÇÃO ===\n");
    printf("Setores aéreos: %d\n", NUM_SETORES);
    printf("Aeronaves: %d\n", NUM_AERONAVES);
    
    if (torre_controle->aeronaves_concluidas > 0) {
        printf("\n=== DESEMPENHO POR PRIORIDADE ===\n");
        printf("Todas as aeronaves completaram suas rotas com segurança!\n");
        printf("Nenhuma colisão ocorreu.\n");
    } else {
        printf("Nenhuma aeronave concluiu a rota.\n");
    }
}

int main(int argc, char *argv[]) {
    // 1. Verificar argumentos 
    if (argc < 3) {
        printf("Uso: %s <num_setores> <num_aeronaves>\n", argv[0]);
        return 1;
    }

    NUM_SETORES = atoi(argv[1]);
    NUM_AERONAVES = atoi(argv[2]);
    
    if (NUM_SETORES <= 0 || NUM_AERONAVES <= 0) {
        printf("Erro: ambos os valores devem ser maiores que 0\n");
        return 1;
    }

    srand(time(NULL)); // Inicializa aleatoriedade

    printf("=== SISTEMA DE CONTROLE DE TRÁFEGO AÉREO ===\n");
    printf("Iniciando Simulação ATC: %d Setores, %d Aeronaves\n", NUM_SETORES, NUM_AERONAVES);

    // TODO: Inicializar a Torre de Controle (MIGUEL)
    torre_controle = inicializar_controle(NUM_SETORES, NUM_AERONAVES);
    
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

        // Pequeno delay entre criação de threads para evitar sincronização perfeita
        usleep(10000); // usleep pausa a execução por microsegundos
    }

    printf("Todas as %d aeronaves foram criadas e iniciaram suas rotas.\n\n", NUM_AERONAVES);

    // 3. Aguardar todas as aeronaves terminarem (pousarem)
    for (int i = 0; i < NUM_AERONAVES; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Simulação finalizada.\n");

    // 4. Imprimir estatísticas finais (tempo médio) 
    imprimir_estatisticas();
    
    // 5. Liberar recursos
    destruir_controle(torre_controle);
    pthread_mutex_destroy(&mutex_log);
    
    printf("\nSimulação finalizada com sucesso!\n");
    
    return 0;
}
