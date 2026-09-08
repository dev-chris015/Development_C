#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CANTIDAD 5

#define SEM_EMPTY_NAME "/sem_empty_lab5"
#define SEM_FULL_NAME  "/sem_full_lab5"

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    sem_unlink(SEM_EMPTY_NAME);
    sem_unlink(SEM_FULL_NAME);

    sem_t *sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem_full = sem_open(SEM_FULL_NAME, O_CREAT | O_EXCL, 0666, 0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        perror("Error al inicializar semáforos");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Error en shmget");
        sem_close(sem_empty);
        sem_close(sem_full);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        exit(EXIT_FAILURE);
    }

    int *buffer = (int *)shmat(shmid, NULL, 0);
    if (buffer == (int *)-1) {
        perror("Error en shmat");
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem_empty);
        sem_close(sem_full);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork");
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem_empty);
        sem_close(sem_full);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        for (int i = 0; i < CANTIDAD; i++) {
            sem_wait(sem_full);

            int val = *buffer;
            printf("Consumidor lee: %d\n", val);

            sem_post(sem_empty);
        }

        shmdt(buffer);
        sem_close(sem_empty);
        sem_close(sem_full);
        _exit(EXIT_SUCCESS);
    } else {
        srand(time(NULL) ^ getpid());

        for (int i = 0; i < CANTIDAD; i++) {
            int num = rand() % 100 + 1;

            sem_wait(sem_empty);

            *buffer = num;

            sem_post(sem_full);
            usleep(50000);
        }

        wait(NULL);

        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);

        sem_close(sem_empty);
        sem_close(sem_full);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
    }

    return EXIT_SUCCESS;
}
