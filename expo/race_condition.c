#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

volatile long shared_counter = 0;
// Inicialización estática del Mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* increment_routine(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++) {
        // entrada a la region critica
        pthread_mutex_lock(&lock);
        shared_counter++;
        pthread_mutex_unlock(&lock);
        // salida de la region critica
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    // Crear dos hilos con verificación de errores
    if (pthread_create(&t1, NULL, increment_routine, NULL) != 0) {
        perror("Error al crear el hilo 1");
        return EXIT_FAILURE;
    }

    if (pthread_create(&t2, NULL, increment_routine, NULL) != 0) {
        perror("Error al crear el hilo 2");
        return EXIT_FAILURE;
    }

    // Esperar a que ambos hilos terminen con verificación de errores
    if (pthread_join(t1, NULL) != 0) {
        perror("Error al unirse al hilo 1");
        return EXIT_FAILURE;
    }

    if (pthread_join(t2, NULL) != 0) {
        perror("Error al unirse al hilo 2");
        return EXIT_FAILURE;
    }

    printf("Valor final del contador: %ld\n", shared_counter);

    // Destruir el mutex
    pthread_mutex_destroy(&lock);
    return EXIT_SUCCESS;
}