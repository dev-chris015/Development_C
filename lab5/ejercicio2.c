#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CICLOS 5

const char *SEM_NAMES[3] = {"/sem_A_lab5", "/sem_B_lab5", "/sem_C_lab5"};

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    sem_t *sem[3];

    for (int i = 0; i < 3; i++) {
        sem_unlink(SEM_NAMES[i]);
    }

    for (int i = 0; i < 3; i++) {
        unsigned int init_val = (i == 0) ? 1 : 0;
        sem[i] = sem_open(SEM_NAMES[i], O_CREAT | O_EXCL, 0666, init_val);
        if (sem[i] == SEM_FAILED) {
            perror("sem_open failed");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork pid1 failed");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        for (int c = 0; c < CICLOS; c++) {
            sem_wait(sem[1]);
            putchar('B');
            fflush(stdout);
            sem_post(sem[2]);
        }
        for (int i = 0; i < 3; i++) {
            sem_close(sem[i]);
        }
        _exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork pid2 failed");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        for (int c = 0; c < CICLOS; c++) {
            sem_wait(sem[2]);
            putchar('C');
            fflush(stdout);
            sem_post(sem[0]);
        }
        for (int i = 0; i < 3; i++) {
            sem_close(sem[i]);
        }
        _exit(EXIT_SUCCESS);
    }

    for (int c = 0; c < CICLOS; c++) {
        sem_wait(sem[0]);
        putchar('A');
        fflush(stdout);
        sem_post(sem[1]);
    }

    wait(NULL);
    wait(NULL);

    printf("\n");

    for (int i = 0; i < 3; i++) {
        sem_close(sem[i]);
        sem_unlink(SEM_NAMES[i]);
    }

    return EXIT_SUCCESS;
}
