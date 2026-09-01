#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    /* Evita que la salida quede duplicada en el buffer despues de fork(). */
    setvbuf(stdout, NULL, _IONBF, 0);

    pid_t hijo1 = fork();
    if (hijo1 < 0) {
        perror("fork del primer hijo");
        return EXIT_FAILURE;
    }

    if (hijo1 == 0) {
        printf("\nHijo 1: PID=%ld, PID del padre=%ld\n",
               (long)getpid(), (long)getppid());
        _exit(EXIT_SUCCESS);
    }

    pid_t hijo2 = fork();
    if (hijo2 < 0) {
        perror("fork del segundo hijo");
        /* El primer hijo ya existe; el padre debe recogerlo. */
        wait(NULL);
        return EXIT_FAILURE;
    }

    if (hijo2 == 0) {
        printf("\nHijo 2: PID=%ld. Ejecutando ls -l con execlp()...\n\n",
               (long)getpid());
        execlp("ls", "ls", "-l", (char *)NULL);

        /* Solo se llega aqui si execlp() falla. */
        perror("execlp");
        _exit(127);
    }

    for (int i = 0; i < 2; i++) {
        int estado;
        pid_t terminado = wait(&estado);

        if (terminado < 0) {
            perror("wait");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(estado)) {
            printf("\nPadre: el hijo PID=%ld termino con codigo %d.\n",
                   (long)terminado, WEXITSTATUS(estado));
        } else if (WIFSIGNALED(estado)) {
            printf("\nPadre: el hijo PID=%ld termino por la senal %d.\n",
                   (long)terminado, WTERMSIG(estado));
        }
    }

    printf("\nTodos los procesos hijos han terminado. Finalizando proceso padre.\n\n");
    return EXIT_SUCCESS;
}
