#include "atc.h"
#include <errno.h> // Para ETIMEDOUT

// Variáveis globais
Controlador controlador;
Aeronave *aeronaves;
int N;
int executando = 1;
time_t inicio_simulacao;

// Protótipos auxiliares locais
void liberar_setor_por_id(int id_setor);
void remover_da_fila(int setor_id, int aeronave_id);

// Inicialização do sistema
void inicializar_sistema(int M, int N_aeronaves) {
    N = N_aeronaves;
    inicio_simulacao = time(NULL);
    
    // Inicializar controlador
    controlador.M = M;
    controlador.setores = (Setor*)malloc(M * sizeof(Setor));
    controlador.fila_espera = (Aeronave**)malloc(M * sizeof(Aeronave*));
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
            // Se o setor não está ocupado E existe alguém na fila
            if (!controlador.setores[i].ocupado && controlador.fila_espera[i] != NULL) {
                // Encontrar aeronave de maior prioridade na fila
                Aeronave *melhor = NULL;
                Aeronave *atual = controlador.fila_espera[i];
                Aeronave *anterior = NULL;
                Aeronave *melhor_anterior = NULL;
                
                // Varre a lista para garantir que pega o maior (caso a inserção tenha falhado na ordem, mas inserção ordenada já ajuda)
                while (atual != NULL) {
                    if (melhor == NULL || atual->prioridade > melhor->prioridade) {
                        melhor = atual;
                        melhor_anterior = anterior;
                    }
                    anterior = atual;
                    atual = atual->prox;
                }
                
                // Remover da fila
                if (melhor != NULL) {
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
                    
                    imprimir_estado(melhor, "CONCEDIDO acesso (via fila)");
                }
            }
        }
        
        pthread_mutex_unlock(&controlador.lock);
        usleep(50000); // 50ms (reduzido para reagir mais rápido)
    }
    return NULL;
}

// Função auxiliar para remover aeronave da fila em caso de timeout
void remover_da_fila(int setor_id, int aeronave_id) {
    Aeronave *atual = controlador.fila_espera[setor_id];
    Aeronave *anterior = NULL;

    while (atual != NULL) {
        if (atual->id == aeronave_id) {
            if (anterior == NULL) {
                controlador.fila_espera[setor_id] = atual->prox;
            } else {
                anterior->prox = atual->prox;
            }
            controlador.tamanho_fila[setor_id]--;
            return;
        }
        anterior = atual;
        atual = atual->prox;
    }
}

// Solicitar acesso a um setor (retorna 1 se sucesso, 0 se timeout)
int solicitar_acesso(Aeronave *av) {
    int setor_destino = av->setor_destino;
    int sucesso = 1; // Assumimos sucesso inicialmente
    
    pthread_mutex_lock(&controlador.lock);
    
    // --- CORREÇÃO 1: ANTI-FURAR-FILA ---
    // Só entra direto se estiver Livre E a fila estiver VAZIA.
    // Se a fila não estiver vazia, eu sou obrigado a entrar na fila para respeitar a prioridade dos que já esperam.
    if (!controlador.setores[setor_destino].ocupado && controlador.fila_espera[setor_destino] == NULL) {
        
        // CASO 1: Setor livre e sem fila - conceder imediatamente
        controlador.setores[setor_destino].ocupado = 1;
        controlador.setores[setor_destino].aeronave_atual = av->id;
        av->setor_atual = setor_destino;
        
        imprimir_estado(av, "CONCEDIDO acesso imediato");
        pthread_mutex_unlock(&controlador.lock);
        return 1;
    } else {
        // CASO 2: Setor ocupado OU com fila - entrar na fila com TIMEOUT
        av->tempo_inicio = time(NULL);
        
        // Inserir na fila mantendo prioridade (Insertion Sort na lista ligada)
        Aeronave *novo = av;
        novo->prox = NULL;
        
        if (controlador.fila_espera[setor_destino] == NULL) {
            controlador.fila_espera[setor_destino] = novo;
        } else {
            Aeronave *atual = controlador.fila_espera[setor_destino];
            Aeronave *anterior = NULL;
            // Procura lugar: enquanto a prioridade do atual for maior, avança.
            while (atual != NULL && atual->prioridade >= novo->prioridade) {
                anterior = atual;
                atual = atual->prox;
            }
            if (anterior == NULL) { // Inserir no início (sou a maior prioridade)
                novo->prox = controlador.fila_espera[setor_destino];
                controlador.fila_espera[setor_destino] = novo;
            } else { // Inserir no meio ou fim
                anterior->prox = novo;
                novo->prox = atual;
            }
        }
        
        controlador.tamanho_fila[setor_destino]++;
        imprimir_estado(av, "AGUARDANDO na fila");
        
        // Timeout de 3 segundos
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 3; 

        // Esperar com timeout
        while (controlador.setores[setor_destino].aeronave_atual != av->id) {
            int err = pthread_cond_timedwait(&av->cond, &controlador.lock, &ts);
            if (err == ETIMEDOUT) {
                if (controlador.setores[setor_destino].aeronave_atual != av->id) {
                    remover_da_fila(setor_destino, av->id);
                    sucesso = 0; // Falha
                    break;
                }
            }
        }
        
        pthread_mutex_unlock(&controlador.lock);
        return sucesso;
    }
}

void liberar_setor_por_id(int id_setor) {
    if (id_setor < 0) return;

    pthread_mutex_lock(&controlador.lock);
    controlador.setores[id_setor].ocupado = 0;
    controlador.setores[id_setor].aeronave_atual = -1;
    // Avisa o controlador que algo mudou
    pthread_cond_signal(&controlador.cond); 
    pthread_mutex_unlock(&controlador.lock);
}

void liberar_setor(Aeronave *av) {
    liberar_setor_por_id(av->setor_atual);
    av->setor_atual = -1;
}

// Thread de uma aeronave
void *thread_aeronave(void *arg) {
    Aeronave *av = (Aeronave*)arg;
    
    int i = 0;
    while (i < av->rota_tam) {
        int setor_anterior = av->setor_atual; 
        av->setor_destino = av->rota[i];
        
        imprimir_estado(av, "SOLICITANDO acesso");
        
        int resultado = solicitar_acesso(av);
        
        if (resultado == 0) {
            // --- CORREÇÃO 2: RETRY DINÂMICO ---
            // Se falhou, o tempo de espera depende da prioridade.
            // Prioridade alta (1000) espera pouco. Prioridade baixa (1) espera mais.
            // Fórmula: Base (100ms) + Penalidade
            
            int sleep_ms = 100 + (1000 - av->prioridade); 
            // Ex: Prio 1000 -> dorme 100ms
            // Ex: Prio 1    -> dorme 1099ms (~1.1s)
            
            imprimir_estado(av, "Aguardando (Prioridade)...");
            usleep(sleep_ms * 1000); 
            
            imprimir_estado(av, "Tentando acesso novamente...");
            resultado = solicitar_acesso(av);
        }

        if (resultado == 1) {
            // SUCESSO
            if (setor_anterior != -1) {
                char msg[50];
                sprintf(msg, "LIBEROU setor %d", setor_anterior);
                imprimir_estado(av, msg);
                liberar_setor_por_id(setor_anterior);
            }
            
            int tempo_voo = (rand() % 3) + 1;
            sleep(tempo_voo);
            
            if (i == av->rota_tam - 1) {
                imprimir_estado(av, "SAINDO do espaço aéreo");
                liberar_setor_por_id(av->setor_atual);
                av->setor_atual = -1;
            }
            i++; 
            
        } else {
            // DEADLOCK / TIMEOUT DUPLO
            imprimir_estado(av, "DEADLOCK DETECTADO: Liberando recursos...");
            
            if (av->setor_atual != -1) {
                liberar_setor_por_id(av->setor_atual);
                av->setor_atual = -1;
            }
            
            // Backoff aleatório maior para resolver o conflito
            usleep(200000 + (rand() % 300000)); 
            
            if (i > 0) i--; 
        }
    }
    
    return NULL;
}

void imprimir_estado(Aeronave *av, const char *acao) {
    time_t agora = time(NULL);
    double tempo_simulacao = difftime(agora, inicio_simulacao);
    
    printf("[T+%.0fs] Aeronave %03d (Pri: %04d) | Setor Atual: %02d | Destino: %02d | %s\n",
           tempo_simulacao, av->id, av->prioridade, 
           av->setor_atual, av->setor_destino, acao);
}

void finalizar_sistema() {
    executando = 0;
    sleep(1); 
    
    printf("\n========== ESTATÍSTICAS FINAIS ==========\n");
    printf("Tempo total de simulação: %.0f segundos\n", 
           difftime(time(NULL), inicio_simulacao));
    
    printf("\nTempos médios de espera por aeronave:\n");
    double soma = 0;
    for (int i = 0; i < N; i++) {
        printf("Aeronave %03d (Pri: %04d): %.2f segundos\n",
               aeronaves[i].id, aeronaves[i].prioridade,
               aeronaves[i].tempo_espera_total);
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