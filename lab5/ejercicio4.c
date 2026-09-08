#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUM_LECTORES 3
#define ITERACIONES 3

#define SEM_MUTEX_NAME "/sem_mutex_lab5_e4"
#define SEM_RW_NAME    "/sem_rw_lab5_e4"

typedef struct {
    pid_t pid_escrito;
    int read_count;
} SharedData;

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_RW_NAME);

    sem_t *sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem_rw = sem_open(SEM_RW_NAME, O_CREAT | O_EXCL, 0666, 1);

    if (sem_mutex == SEM_FAILED || sem_rw == SEM_FAILED) {
        perror("Error al inicializar semáforos");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Error en shmget");
        sem_close(sem_mutex);
        sem_close(sem_rw);
        sem_unlink(SEM_MUTEX_NAME);
        sem_unlink(SEM_RW_NAME);
        exit(EXIT_FAILURE);
    }

    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
    if (data == (SharedData *)-1) {
        perror("Error en shmat");
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem_mutex);
        sem_close(sem_rw);
        sem_unlink(SEM_MUTEX_NAME);
        sem_unlink(SEM_RW_NAME);
        exit(EXIT_FAILURE);
    }

    data->pid_escrito = 0;
    data->read_count = 0;

    pid_t pid_escritor = fork();
    if (pid_escritor < 0) {
        perror("Error en fork escritor");
        exit(EXIT_FAILURE);
    }

    if (pid_escritor == 0) {
        for (int i = 0; i < ITERACIONES; i++) {
            sem_wait(sem_rw);

            data->pid_escrito = getpid();
            printf("[Escritor PID=%d]: Escribió PID=%d en memoria compartida (Iteración %d)\n",
                   getpid(), getpid(), i + 1);
            usleep(100000);

            sem_post(sem_rw);
            usleep(150000);
        }

        shmdt(data);
        sem_close(sem_mutex);
        sem_close(sem_rw);
        _exit(EXIT_SUCCESS);
    }

    pid_t pid_lectores[NUM_LECTORES];
    for (int i = 0; i < NUM_LECTORES; i++) {
        pid_lectores[i] = fork();
        if (pid_lectores[i] < 0) {
            perror("Error en fork lector");
            exit(EXIT_FAILURE);
        }

        if (pid_lectores[i] == 0) {
            for (int k = 0; k < ITERACIONES; k++) {
                usleep(50000);

                sem_wait(sem_mutex);
                data->read_count++;
                if (data->read_count == 1) {
                    sem_wait(sem_rw);
                }
                sem_post(sem_mutex);

                printf("[Lector %d PID=%d]: Leyó PID=%d de memoria compartida (Lectores activos: %d)\n",
                       i + 1, getpid(), data->pid_escrito, data->read_count);
                usleep(80000);

                sem_wait(sem_mutex);
                data->read_count--;
                if (data->read_count == 0) {
                    sem_post(sem_rw);
                }
                sem_post(sem_mutex);

                usleep(100000);
            }

            shmdt(data);
            sem_close(sem_mutex);
            sem_close(sem_rw);
            _exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < NUM_LECTORES + 1; i++) {
        wait(NULL);
    }

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    sem_close(sem_mutex);
    sem_close(sem_rw);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_RW_NAME);

    return EXIT_SUCCESS;
}
