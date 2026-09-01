#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define LLAVE 34857
#define MAX_TEXTO 256
#define RONDAS 20
#define PARA_RECEPTOR 1L
#define PARA_EMISOR 2L

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
    int cola = msgget((key_t)LLAVE, 0660);
    if (cola == -1) {
        perror("msgget (ejecute primero ./emisor)");
        return EXIT_FAILURE;
    }

    printf("\nReceptor conectado al emisor.\n\n");

    for (int ronda = 1; ronda <= RONDAS; ronda++) {
        struct mensaje recibido;
        char respuesta[MAX_TEXTO];

        if (!recibir(cola, PARA_RECEPTOR, &recibido)) {
            return EXIT_FAILURE;
        }
        printf("Emisor [%d/%d]: %s\n", ronda, RONDAS, recibido.texto);
        if (strcmp(recibido.texto, "salir") == 0) {
            /* Confirma la salida antes de que el emisor elimine la cola. */
            if (!enviar(cola, PARA_EMISOR, "salir")) {
                return EXIT_FAILURE;
            }
            break;
        }

        printf("Receptor [%d/%d]> ", ronda, RONDAS);
        fflush(stdout);
        if (!leer_linea(respuesta, sizeof respuesta)) {
            snprintf(respuesta, sizeof respuesta, "salir");
        }
        if (!enviar(cola, PARA_EMISOR, respuesta)) {
            return EXIT_FAILURE;
        }
        if (strcmp(respuesta, "salir") == 0) {
            break;
        }
    }

    printf("\nReceptor: comunicacion finalizada.\n\n");
    return EXIT_SUCCESS;
}
