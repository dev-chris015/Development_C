#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Paso 1: Configurar la Ruta del Archivo de Entrada
    const char *archivo_path = "/home/devchris/Descargas/cadenas.txt";
    if (argc > 1) {
        archivo_path = argv[1];
    }

    FILE *archivo = fopen(archivo_path, "r");
    if (!archivo) {
        perror("Error al abrir el archivo");
        return 1;
    }

    // Paso 2: Arreglo Dinámico de Punteros
    char **cadenas = NULL;
    size_t capacidad = 0;
    size_t total_cadenas = 0;

    // Variables para la lectura con getline
    char *linea = NULL;
    size_t len = 0;
    ssize_t leidos;

    // Variables para estadísticas
    size_t total_bytes_cadenas = 0;
    size_t max_longitud = 0;
    char *cadena_mas_larga = NULL;

    printf("--- Procesando líneas del archivo: %s ---\n", archivo_path);
    
    // Lectura Dinámica
    while ((leidos = getline(&linea, &len, archivo)) != -1) {
        // Eliminar salto de línea si existe
        if (leidos > 0 && linea[leidos - 1] == '\n') {
            linea[leidos - 1] = '\0';
            leidos--;
        }

        // Redimensionar el arreglo de punteros si es necesario
        if (total_cadenas >= capacidad) {
            capacidad = (capacidad == 0) ? 10 : capacidad * 2;
            char **nuevo_arreglo = realloc(cadenas, capacidad * sizeof(char *));
            if (!nuevo_arreglo) {
                perror("Error de asignación de memoria para el arreglo de punteros");
                break;
            }
            cadenas = nuevo_arreglo;
        }

        // Asignar memoria exacta para la cadena
        size_t bytes_asignados = leidos + 1; // +1 para el carácter nulo '\0'
        cadenas[total_cadenas] = malloc(bytes_asignados);
        
        // Verificar asignación válida
        if (!cadenas[total_cadenas]) {
            perror("Error de asignación para la cadena");
            break;
        }
        
        // Guardar la cadena
        strcpy(cadenas[total_cadenas], linea);
        total_bytes_cadenas += bytes_asignados;

        // Determinar la cadena más larga
        if (leidos > max_longitud || total_cadenas == 0) {
            max_longitud = leidos;
            cadena_mas_larga = cadenas[total_cadenas];
        }

        // Imprimir contenido, tamaño en bytes y dirección de memoria
        printf("Cadena %zu: '%s' | Tamaño: %zu bytes | Dirección: %p\n",
               total_cadenas + 1, cadenas[total_cadenas], bytes_asignados, (void *)cadenas[total_cadenas]);

        total_cadenas++;
    }

    // Liberar el buffer usado por getline y cerrar archivo
    free(linea);
    fclose(archivo);

    // Cálculo y muestra de Estadísticas
    printf("\n--- Estadísticas ---\n");
    size_t bytes_arreglo_punteros = capacidad * sizeof(char *);
    printf("Total de cadenas procesadas: %zu\n", total_cadenas);
    printf("Bytes asignados para las cadenas: %zu\n", total_bytes_cadenas);
    printf("Bytes asignados para el arreglo de punteros (capacidad %zu): %zu\n", capacidad, bytes_arreglo_punteros);
    printf("Total general de bytes asignados dinámicamente: %zu\n", total_bytes_cadenas + bytes_arreglo_punteros);

    if (total_cadenas > 0) {
        printf("Cadena más larga: '%s' (Longitud: %zu caracteres)\n", cadena_mas_larga, max_longitud);
        printf("Rango de direcciones: desde %p hasta %p\n",
               (void *)cadenas[0], (void *)cadenas[total_cadenas - 1]);
    } else {
        printf("No se procesaron cadenas.\n");
    }

    // Liberar toda la memoria dinámica antes de salir
    for (size_t i = 0; i < total_cadenas; i++) {
        free(cadenas[i]);
    }
    free(cadenas);

    return 0;
}
