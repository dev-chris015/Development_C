#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    pid_t hijo = fork();
    if (hijo < 0) {
        perror("fork del hijo");
        return EXIT_FAILURE;
    }

    if (hijo == 0) {
        pid_t nieto = fork();
        if (nieto < 0) {
            perror("fork del nieto");
            _exit(EXIT_FAILURE);
        }

        if (nieto == 0) {
            printf("NIETO: PID=%ld, PID de su padre (Hijo)=%ld\n",
                   (long)getpid(), (long)getppid());
            sleep(15);
            _exit(EXIT_SUCCESS);
        }

        printf("HIJO: PID=%ld, PID de su padre (Padre)=%ld, PID de su hijo (Nieto)=%ld\n",
               (long)getpid(), (long)getppid(), (long)nieto);
        sleep(15);

        if (waitpid(nieto, NULL, 0) < 0) {
            perror("waitpid del nieto");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }

    printf("PADRE: PID=%ld, PID de su creador=%ld, PID de su hijo=%ld\n",
           (long)getpid(), (long)getppid(), (long)hijo);
    printf("Durante 15 segundos ejecute en otra terminal: pstree -p %ld\n",
           (long)getpid());
    sleep(15);

    if (waitpid(hijo, NULL, 0) < 0) {
        perror("waitpid del hijo");
        return EXIT_FAILURE;
    }

    printf("PADRE: la jerarquia completa ha terminado.\n");
    return EXIT_SUCCESS;
}
