#include "atc.h"

// Variáveis globais
Controlador controlador;
Aeronave *aeronaves;
int N;
int executando = 1;
time_t inicio_simulacao;

// Inicialização do sistema
void inicializar_sistema(int M, int N_aeronaves) {
    N = N_aeronaves;
    inicio_simulacao = time(NULL);
    
    // Inicializar controlador
    controlador.M = M;
    controlador.setores = (Setor*)malloc(M * sizeof(Setor));
    controlador.fila_espera = (Aeronave*)malloc(M * sizeof(Aeronave));
    controlador.tamanho_fila = (int*)calloc(M, sizeof(int));
    
    pthread_mutex_init(&controlador.lock, NULL);
    pthread_cond_init(&controlador.cond, NULL);
    
    // Inicializar setores
    for (int i = 0; i < M; i++) {
        controlador.setores[i].id = i;
        controlador.setores[i].ocupado = 0;
        controlador.setores[i].aeronave_atual = -1;
        pthread_mutex_init(&controlador.setores[i].lock, NULL);
        pthread_cond_init(&controlador.setores[i].cond, NULL);
        controlador.fila_espera[i] = NULL;
    }
    
    // Inicializar aeronaves
    aeronaves = (Aeronave*)malloc(N * sizeof(Aeronave));
    srand(time(NULL));
    
    for (int i = 0; i < N; i++) {
        aeronaves[i].id = i;
        aeronaves[i].prioridade = (rand() % 1000) + 1; // 1-1000
        aeronaves[i].setor_atual = -1; // Começa fora do espaço aéreo
        
        // Gerar rota aleatória (3-6 setores)
        aeronaves[i].rota_tam = (rand() % 4) + 3;
        aeronaves[i].rota = (int*)malloc(aeronaves[i].rota_tam * sizeof(int));
        
        // Gerar rota não repetida
        for (int j = 0; j < aeronaves[i].rota_tam; j++) {
            int setor;
            do {
                setor = rand() % M;
            } while (j > 0 && aeronaves[i].rota[j-1] == setor);
            aeronaves[i].rota[j] = setor;
        }
        
        aeronaves[i].setor_destino = aeronaves[i].rota[0];
        aeronaves[i].tempo_espera_total = 0;
        aeronaves[i].prox = NULL;
        pthread_cond_init(&aeronaves[i].cond, NULL);
    }
}

// Thread de controle centralizado
void *thread_controlador(void *arg) {
    while (executando) {
        pthread_mutex_lock(&controlador.lock);
        
        // Verificar cada setor
        for (int i = 0; i < controlador.M; i++) {
            if (!controlador.setores[i].ocupado && controlador.fila_espera[i] != NULL) {
                // Encontrar aeronave de maior prioridade na fila
                Aeronave *melhor = NULL;
                Aeronave *atual = controlador.fila_espera[i];
                Aeronave *anterior = NULL;
                Aeronave *melhor_anterior = NULL;
                
                while (atual != NULL) {
                    if (melhor == NULL || atual->prioridade > melhor->prioridade) {
                        melhor = atual;
                        melhor_anterior = anterior;
                    }
                    anterior = atual;
                    atual = atual->prox;
                }
                
                // Remover da fila
                if (melhor_anterior == NULL) {
                    controlador.fila_espera[i] = melhor->prox;
                } else {
                    melhor_anterior->prox = melhor->prox;
                }
                controlador.tamanho_fila[i]--;
                
                // Conceder acesso
                controlador.setores[i].ocupado = 1;
                controlador.setores[i].aeronave_atual = melhor->id;
                melhor->setor_atual = i;
                
                time_t agora = time(NULL);
                melhor->tempo_espera_total += difftime(agora, melhor->tempo_inicio);
                
                // Acordar aeronave
                pthread_cond_signal(&melhor->cond);
                
                imprimir_estado(melhor, "CONCEDIDO acesso ao setor");
            }
        }
        
        pthread_mutex_unlock(&controlador.lock);
        usleep(100000); // 100ms
    }
    return NULL;
}

// Solicitar acesso a um setor
void solicitar_acesso(Aeronave *av) {
    int setor_destino = av->setor_destino;
    
    pthread_mutex_lock(&controlador.lock);
    
    if (!controlador.setores[setor_destino].ocupado) {
        // Setor livre - conceder imediatamente
        controlador.setores[setor_destino].ocupado = 1;
        controlador.setores[setor_destino].aeronave_atual = av->id;
        av->setor_atual = setor_destino;
        
        time_t agora = time(NULL);
        av->tempo_espera_total += difftime(agora, av->tempo_inicio);
        
        imprimir_estado(av, "CONCEDIDO acesso imediato");
        pthread_mutex_unlock(&controlador.lock);
    } else {
        // Setor ocupado - entrar na fila de espera
        av->tempo_inicio = time(NULL);
        
        // Inserir na fila mantendo prioridade
        Aeronave *novo = av;
        novo->prox = NULL;
        
        if (controlador.fila_espera[setor_destino] == NULL) {
            controlador.fila_espera[setor_destino] = novo;
        } else {
            // Inserir ordenado por prioridade (decrescente)
            Aeronave *atual = controlador.fila_espera[setor_destino];
            Aeronave *anterior = NULL;
            
            while (atual != NULL && atual->prioridade > novo->prioridade) {
                anterior = atual;
                atual = atual->prox;
            }
            
            if (anterior == NULL) {
                novo->prox = controlador.fila_espera[setor_destino];
                controlador.fila_espera[setor_destino] = novo;
            } else {
                anterior->prox = novo;
                novo->prox = atual;
            }
        }
        
        controlador.tamanho_fila[setor_destino]++;
        imprimir_estado(av, "AGUARDANDO na fila");
        
        // Esperar até ser atendido
        while (controlador.setores[setor_destino].aeronave_atual != av->id) {
            pthread_cond_wait(&av->cond, &controlador.lock);
        }
        
        pthread_mutex_unlock(&controlador.lock);
    }
}

// Liberar setor atual
void liberar_setor(Aeronave *av) {
    if (av->setor_atual >= 0) {
        pthread_mutex_lock(&controlador.lock);
        
        int setor_atual = av->setor_atual;
        controlador.setores[setor_atual].ocupado = 0;
        controlador.setores[setor_atual].aeronave_atual = -1;
        
        imprimir_estado(av, "LIBEROU setor");
        
        // Sinalizar para o controlador verificar a fila
        pthread_cond_signal(&controlador.cond);
        pthread_mutex_unlock(&controlador.lock);
        
        av->setor_atual = -1;
    }
}

// Thread de uma aeronave
void *thread_aeronave(void *arg) {
    Aeronave *av = (Aeronave*)arg;
    
    for (int i = 0; i < av->rota_tam; i++) {
        av->setor_destino = av->rota[i];
        
        // 1. Solicitar acesso ao próximo setor
        imprimir_estado(av, "SOLICITANDO acesso");
        solicitar_acesso(av);
        
        // 2. Se não é o primeiro setor, liberar o anterior
        if (i > 0) {
            liberar_setor(av);
        }
        
        // 3. Simular voo no setor (1-3 segundos)
        int tempo_voo = (rand() % 3) + 1;
        sleep(tempo_voo);
        
        // 4. Se é o último setor, liberar antes de sair
        if (i == av->rota_tam - 1) {
            liberar_setor(av);
        }
    }
    
    return NULL;
}

// Imprimir estado atual
void imprimir_estado(Aeronave *av, const char *acao) {
    time_t agora = time(NULL);
    double tempo_simulacao = difftime(agora, inicio_simulacao);
    
    printf("[T+%.0fs] Aeronave %03d (Pri: %04d) | Setor Atual: %02d | Destino: %02d | %s\n",
           tempo_simulacao, av->id, av->prioridade, 
           av->setor_atual, av->setor_destino, acao);
}

// Finalizar sistema e calcular estatísticas
void finalizar_sistema() {
    executando = 0;
    sleep(2); // Aguardar threads terminarem
    
    printf("\n========== ESTATÍSTICAS FINAIS ==========\n");
    printf("Tempo total de simulação: %.0f segundos\n", 
           difftime(time(NULL), inicio_simulacao));
    
    printf("\nTempos médios de espera por aeronave:\n");
    for (int i = 0; i < N; i++) {
        printf("Aeronave %03d (Pri: %04d): %.2f segundos\n",
               aeronaves[i].id, aeronaves[i].prioridade,
               aeronaves[i].tempo_espera_total);
    }
    
    // Calcular média geral
    double soma = 0;
    for (int i = 0; i < N; i++) {
        soma += aeronaves[i].tempo_espera_total;
    }
    printf("\nTempo médio de espera geral: %.2f segundos\n", soma / N);
    
    // Liberar memória
    for (int i = 0; i < N; i++) {
        free(aeronaves[i].rota);
    }
    free(aeronaves);
    free(controlador.setores);
    free(controlador.fila_espera);
    free(controlador.tamanho_fila);
}