#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define NUM_TURNOS 5
#define CARACTERES_POR_TURNO 10

#define SEM_PADRE_NAME "/sem_padre_lab5"
#define SEM_HIJO_NAME  "/sem_hijo_lab5"

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    sem_unlink(SEM_PADRE_NAME);
    sem_unlink(SEM_HIJO_NAME);

    sem_t *sem_padre = sem_open(SEM_PADRE_NAME, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem_hijo = sem_open(SEM_HIJO_NAME, O_CREAT | O_EXCL, 0666, 0);

    if (sem_padre == SEM_FAILED || sem_hijo == SEM_FAILED) {
        perror("Error al inicializar los semáforos");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al ejecutar fork()");
        sem_close(sem_padre);
        sem_close(sem_hijo);
        sem_unlink(SEM_PADRE_NAME);
        sem_unlink(SEM_HIJO_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        for (int turno = 0; turno < NUM_TURNOS; turno++) {
            sem_wait(sem_hijo);

            printf("Hijo  (Turno %d): ", turno + 1);
            for (int i = 0; i < CARACTERES_POR_TURNO; i++) {
                putchar('o');
            }
            printf("\n");

            sem_post(sem_padre);
        }

        sem_close(sem_padre);
        sem_close(sem_hijo);
        _exit(EXIT_SUCCESS);
    } else {
        for (int turno = 0; turno < NUM_TURNOS; turno++) {
            sem_wait(sem_padre);

            printf("Padre (Turno %d): ", turno + 1);
            for (int i = 0; i < CARACTERES_POR_TURNO; i++) {
                putchar('+');
            }
            printf("\n");

            sem_post(sem_hijo);
        }

        if (wait(NULL) < 0) {
            perror("Error en wait()");
        }

        sem_close(sem_padre);
        sem_close(sem_hijo);
        sem_unlink(SEM_PADRE_NAME);
        sem_unlink(SEM_HIJO_NAME);
    }

    return EXIT_SUCCESS;
}
