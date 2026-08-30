#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int ready_flag = 0; // Condición compartida

void* worker_thread(void* arg) {
    (void)arg;
    printf("[Hilo Trabajador] Iniciando, pero esperando la señal para continuar...\n");
    
    pthread_mutex_lock(&mutex);
    while (ready_flag == 0) {
        // El hilo se duerme eficientemente liberando el mutex mientras espera
        pthread_cond_wait(&cond, &mutex);
    }
    printf("[Hilo Trabajador] ¡Desperté! La condición se cumplió.\n");
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(void) {
    pthread_t worker;

    if (pthread_create(&worker, NULL, worker_thread, NULL) != 0) {
        perror("Error al crear el hilo trabajador");
        return EXIT_FAILURE;
    }

    printf("[Hilo Principal] Preparando recursos durante 3 segundos...\n");
    sleep(3);

    pthread_mutex_lock(&mutex);
    ready_flag = 1; // Cambiamos la condición
    printf("[Hilo Principal] Enviando señal para despertar al trabajador...\n");
    pthread_cond_signal(&cond); // Despertar al hilo dormido
    pthread_mutex_unlock(&mutex);

    if (pthread_join(worker, NULL) != 0) {
        perror("Error al unirse al hilo trabajador");
        return EXIT_FAILURE;
    }
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return EXIT_SUCCESS;
}