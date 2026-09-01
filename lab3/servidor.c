#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define LLAVE 34856
#define MAX_TEXTO 256
#define RONDAS 20
#define PARA_CLIENTE 1L
#define PARA_SERVIDOR 2L

struct mensaje {
    long tipo;
    char texto[MAX_TEXTO];
};

static int leer_linea(char *destino, size_t capacidad) {
    if (fgets(destino, (int)capacidad, stdin) == NULL) {
        return 0;
    }
    destino[strcspn(destino, "\n")] = '\0';
    return 1;
}

static int enviar(int cola, long tipo, const char *texto) {
    struct mensaje msg = {.tipo = tipo};
    snprintf(msg.texto, sizeof msg.texto, "%s", texto);
    if (msgsnd(cola, &msg, strlen(msg.texto) + 1, 0) == -1) {
        perror("msgsnd");
        return 0;
    }
    return 1;
}

static int recibir(int cola, long tipo, struct mensaje *msg) {
    ssize_t bytes = msgrcv(cola, msg, sizeof msg->texto, tipo, 0);
    if (bytes == -1) {
        perror("msgrcv");
        return 0;
    }
    msg->texto[MAX_TEXTO - 1] = '\0';
    return 1;
}

int main(void) {
    int cola = msgget((key_t)LLAVE, IPC_CREAT | IPC_EXCL | 0660);
    if (cola == -1) {
        if (errno == EEXIST) {
            fprintf(stderr,
                    "Ya existe la cola %d. Elimine una cola anterior con ipcrm -Q %d y reintente.\n",
                    LLAVE, LLAVE);
        } else {
            perror("msgget");
        }
        return EXIT_FAILURE;
    }

    printf("\nServidor listo. Abra otra terminal y ejecute ./cliente\n");
    printf("Se realizaran hasta %d rondas (20 mensajes de cada lado).\n\n", RONDAS);

    for (int ronda = 1; ronda <= RONDAS; ronda++) {
        char texto[MAX_TEXTO];
        struct mensaje respuesta;

        printf("Servidor [%d/%d]> ", ronda, RONDAS);
        fflush(stdout);
        if (!leer_linea(texto, sizeof texto)) {
            snprintf(texto, sizeof texto, "salir");
        }

        if (!enviar(cola, PARA_CLIENTE, texto)) {
            msgctl(cola, IPC_RMID, NULL);
            return EXIT_FAILURE;
        }
        if (!recibir(cola, PARA_SERVIDOR, &respuesta)) {
            msgctl(cola, IPC_RMID, NULL);
            return EXIT_FAILURE;
        }
        printf("Cliente [%d/%d]: %s\n", ronda, RONDAS, respuesta.texto);
        if (strcmp(texto, "salir") == 0 || strcmp(respuesta.texto, "salir") == 0) {
            break;
        }
    }

    if (msgctl(cola, IPC_RMID, NULL) == -1) {
        perror("msgctl(IPC_RMID)");
        return EXIT_FAILURE;
    }
    printf("\nServidor: conversacion finalizada y cola eliminada.\n\n");
    return EXIT_SUCCESS;
}
