#define OPENSSL_SUPPRESS_DEPRECATED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <openssl/md5.h>

// Estructura de información para cada archivo y directorio
typedef struct FileInfo {
    char path[1024];          // Ruta completa o relativa
    char name[256];           // Nombre del archivo
    off_t size;               // Tamaño en bytes
    ino_t inode;              // Número de inodo
    mode_t mode;              // Permisos (st_mode)
    uid_t uid;                // ID del propietario
    gid_t gid;                // ID del grupo
    time_t mtime;             // Fecha de modificación
    char hash[33];            // Hash MD5 (si se solicita -h)
    int is_dir;               // Indicador si es directorio
    struct FileInfo *next;    // Para lista enlazada
} FileInfo;

// Configuración de opciones pasadas por línea de comandos
typedef struct Options {
    int show_inode;           // Bandera -i (inodo)
    int show_perms;           // Bandera -p (permisos y propietario/grupo)
    int show_size;            // Bandera -s (tamaño legible)
    int show_hash;            // Bandera -h (hash MD5)
    int detect_duplicates;    // Bandera -d (detección de duplicados)
} Options;

// Convierte el tamaño en bytes a un formato legible (B, KB, MB, GB, TB)
void human_readable_size(off_t bytes, char *buffer, size_t buf_size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = (double)bytes;
    int unit_index = 0;

    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }

    if (unit_index == 0) {
        snprintf(buffer, buf_size, "%ldB", (long)bytes);
    } else {
        snprintf(buffer, buf_size, "%.1f%s", size, units[unit_index]);
    }
}

// Traduce los bits de permiso (st_mode) a una cadena tipo rwxr-xr-x
void permissions_to_string(mode_t mode, char *str) {
    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else str[0] = '-';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

// Calcula el hash MD5 de un archivo leyendo en bloques usando OpenSSL
int get_file_hash(const char *filepath, char *hash_out) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        snprintf(hash_out, 33, "ERROR");
        return -1;
    }

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    unsigned char buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        MD5_Update(&md5_ctx, buffer, bytes_read);
    }
    fclose(file);

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5_ctx);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(hash_out + (i * 2), 3, "%02x", digest[i]);
    }
    hash_out[32] = '\0';
    return 0;
}

// Crea y llena dinámicamente una nueva estructura FileInfo
FileInfo *create_file_node(const char *path, const char *name, struct stat *st, int is_dir) {
    FileInfo *node = (FileInfo *)malloc(sizeof(FileInfo));
    if (!node) {
        perror("Error al asignar memoria");
        return NULL;
    }

    snprintf(node->path, sizeof(node->path), "%s", path);
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->size = st->st_size;
    node->inode = st->st_ino;
    node->mode = st->st_mode;
    node->uid = st->st_uid;
    node->gid = st->st_gid;
    node->mtime = st->st_mtime;
    node->is_dir = is_dir;
    node->hash[0] = '\0';
    node->next = NULL;

    return node;
}

// Agrega un nodo al inicio de la lista simplemente enlazada
void add_file_node(FileInfo **head, FileInfo *node) {
    if (!node) return;
    node->next = *head;
    *head = node;
}

// Libera toda la memoria reservada para la lista enlazada
void free_file_list(FileInfo *head) {
    FileInfo *current = head;
    while (current) {
        FileInfo *tmp = current;
        current = current->next;
        free(tmp);
    }
}

// Aplica indentación gráfica basada en la profundidad simulando el comando tree
void print_indentation(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("│   ");
    }
}

// Formatea los metadatos a imprimir según las opciones seleccionadas (-s, -i, -p, -h)
void format_file_details(FileInfo *node, Options *opts, char *details, size_t max_size) {
    details[0] = '\0';
    char buffer[256];

    if (opts->show_size) {
        char size_str[32];
        human_readable_size(node->size, size_str, sizeof(size_str));
        snprintf(buffer, sizeof(buffer), " [%s]", size_str);
        strncat(details, buffer, max_size - strlen(details) - 1);
    }

    if (opts->show_inode) {
        snprintf(buffer, sizeof(buffer), " [ino: %lu]", (unsigned long)node->inode);
        strncat(details, buffer, max_size - strlen(details) - 1);
    }

    if (opts->show_perms) {
        char perm_str[12];
        permissions_to_string(node->mode, perm_str);
        struct passwd *pw = getpwuid(node->uid);
        struct group *gr = getgrgid(node->gid);
        const char *user = pw ? pw->pw_name : "desconocido";
        const char *group = gr ? gr->gr_name : "desconocido";
        snprintf(buffer, sizeof(buffer), " [%s %s:%s]", perm_str, user, group);
        strncat(details, buffer, max_size - strlen(details) - 1);
    }

    if (opts->show_hash && !node->is_dir) {
        snprintf(buffer, sizeof(buffer), " [md5: %s]", node->hash[0] ? node->hash : "N/A");
        strncat(details, buffer, max_size - strlen(details) - 1);
    }
}

// Fase 2: Función recursiva para recorrer directorios y extraer metadatos
void analyze_directory(const char *base_path, int depth, Options *opts, FileInfo **file_list, int *total_files, int *total_dirs) {
    DIR *dir = opendir(base_path);
    if (!dir) {
        print_indentation(depth);
        printf("└── [Error al abrir directorio: %s]\n", base_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar los directorios especiales . y ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construir la ruta completa del archivo o subdirectorio
        char full_path[1024];
        if (strcmp(base_path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->d_name);
        }

        // Obtener metadatos sin seguir enlaces simbólicos mediante lstat()
        struct stat file_stat;
        if (lstat(full_path, &file_stat) != 0) {
            print_indentation(depth);
            printf("├── %s [Error en lstat]\n", entry->d_name);
            continue;
        }

        int is_dir = S_ISDIR(file_stat.st_mode);
        FileInfo *node = create_file_node(full_path, entry->d_name, &file_stat, is_dir);

        if (!is_dir) {
            (*total_files)++;
            // Si la bandera -h o -d esta activa, se calcula el hash MD5
            if (opts->show_hash || opts->detect_duplicates) {
                get_file_hash(full_path, node->hash);
            }
            add_file_node(file_list, node);
        } else {
            (*total_dirs)++;
        }

        // Impresión gráfica de la estructura en árbol
        print_indentation(depth);
        char details[512];
        format_file_details(node, opts, details, sizeof(details));
        printf("├── %s%s\n", entry->d_name, details);

        // Llamada recursiva para subdirectorios
        if (is_dir) {
            analyze_directory(full_path, depth + 1, opts, file_list, total_files, total_dirs);
        }
    }

    closedir(dir);
}

// Fase 4: Algoritmo de agrupación y reporte de archivos duplicados por inodo o hash MD5
void process_duplicates(FileInfo *head, int total_files) {
    printf("\n==================================================\n");
    printf("RESUMEN DE DETECCIÓN DE DUPLICADOS\n");
    printf("==================================================\n");
    printf("Total de archivos analizados: %d\n", total_files);

    int file_count = 0;
    FileInfo *curr = head;
    while (curr) {
        file_count++;
        curr = curr->next;
    }

    if (file_count == 0) {
        printf("No hay archivos para analizar duplicados.\n");
        return;
    }

    FileInfo **files = (FileInfo **)malloc(file_count * sizeof(FileInfo *));
    if (!files) {
        perror("Error al asignar memoria para arreglo de duplicados");
        return;
    }

    curr = head;
    for (int i = 0; i < file_count; i++) {
        files[i] = curr;
        curr = curr->next;
    }

    int *visited = (int *)calloc(file_count, sizeof(int));
    int duplicate_groups = 0;
    int total_duplicates_count = 0;

    for (int i = 0; i < file_count; i++) {
        if (visited[i]) continue;

        int group_count = 0;
        for (int j = i + 1; j < file_count; j++) {
            if (visited[j]) continue;

            int is_match = 0;
            // Coincidencia por inodo (enlaces duros) o por hash MD5
            if (files[i]->inode == files[j]->inode) {
                is_match = 1;
            } else if (files[i]->hash[0] != '\0' && strcmp(files[i]->hash, "ERROR") != 0 &&
                       strcmp(files[i]->hash, files[j]->hash) == 0) {
                is_match = 1;
            }

            if (is_match) {
                if (group_count == 0) {
                    duplicate_groups++;
                    char size_str[32];
                    human_readable_size(files[i]->size, size_str, sizeof(size_str));
                    printf("\nGrupo %d (Hash: %s, Tamaño: %s):\n", duplicate_groups,
                           files[i]->hash[0] ? files[i]->hash : "N/A", size_str);
                    printf("  - %s (Inodo: %lu)\n", files[i]->path, (unsigned long)files[i]->inode);
                    visited[i] = 1;
                    group_count++;
                    total_duplicates_count++;
                }

                printf("  - %s (Inodo: %lu)\n", files[j]->path, (unsigned long)files[j]->inode);
                visited[j] = 1;
                group_count++;
                total_duplicates_count++;
            }
        }
    }

    printf("\nGrupos de archivos duplicados encontrados: %d\n", duplicate_groups);
    printf("Total de archivos duplicados (incluyendo copias): %d\n", total_duplicates_count);

    free(files);
    free(visited);
}

// Muestra el mensaje de ayuda de uso del programa
void print_usage(const char *prog_name) {
    printf("Uso: %s [opciones] <directorio>\n", prog_name);
    printf("Opciones:\n");
    printf("  -i  Mostrar número de inodo\n");
    printf("  -p  Mostrar permisos y propietario/grupo\n");
    printf("  -s  Mostrar tamaño de archivo legible (human-readable)\n");
    printf("  -h  Calcular y mostrar hash MD5\n");
    printf("  -d  Detectar y mostrar archivos duplicados\n");
}

// Punto de entrada principal y análisis de opciones por línea de comandos con getopt()
int main(int argc, char *argv[]) {
    Options opts = {0, 0, 0, 0, 0};
    int opt;

    // Procesamiento de banderas pasadas por consola
    while ((opt = getopt(argc, argv, "ipshd")) != -1) {
        switch (opt) {
            case 'i': opts.show_inode = 1; break;
            case 'p': opts.show_perms = 1; break;
            case 's': opts.show_size = 1; break;
            case 'h': opts.show_hash = 1; break;
            case 'd': opts.detect_duplicates = 1; break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Validación del argumento del directorio objetivo
    if (optind >= argc) {
        fprintf(stderr, "Error: Debe especificar un directorio objetivo.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *target_dir = argv[optind];
    struct stat st;
    if (stat(target_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: El directorio '%s' no existe o no es valido.\n", target_dir);
        return EXIT_FAILURE;
    }

    FileInfo *file_list = NULL;
    int total_files = 0;
    int total_dirs = 0;

    printf("Analizando directorio: %s\n", target_dir);
    analyze_directory(target_dir, 0, &opts, &file_list, &total_files, &total_dirs);

    printf("\nResumen general:\n");
    printf("Directorios analizados: %d\n", total_dirs);
    printf("Archivos analizados: %d\n", total_files);

    // Si la opción -d esta activada, procesar y mostrar duplicados
    if (opts.detect_duplicates) {
        process_duplicates(file_list, total_files);
    }

    free_file_list(file_list);
    return EXIT_SUCCESS;
}
