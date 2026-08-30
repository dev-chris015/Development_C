#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

long shared_counter = 0;
pthread_mutex_t lock; // Declaración del Mutex para exclusión mutua

void* increment_routine(void* arg) {
    for (int i = 0; i < 1000000; i++) {
        // inicio region critica
        pthread_mutex_lock(&lock);
        shared_counter++;
        pthread_mutex_unlock(&lock);
        //fin
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    // Inicializar el mutex
    pthread_mutex_init(&lock, NULL);

    // Crear dos hilos que ejecutarán la misma rutina
    pthread_create(&t1, NULL, increment_routine, NULL);
    pthread_create(&t2, NULL, increment_routine, NULL);

    // Esperar a que ambos hilos terminen
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Valor final del contador: %ld (Esperado: 2000000)\n", shared_counter);

    // Destruir el mutex
    pthread_mutex_destroy(&lock);
    return 0;
}