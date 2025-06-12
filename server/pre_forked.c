/*
* PROYECTO PROGRAMADO 02
* SISTEMAS OPERATIVOS
* ESTUDIANTES:
* Ignacio Rodríguez Ovares, 118940243
* David Núñez Franco, 119080008
* Sharon María Araya Ramírez, 402530296
* CREATED 24/05/2025
*/

/*
 * Implementación del modo PRE-FORKED
 * Se crean K procesos hijos al inicio. Cada uno se bloquea en accept().
 * Cuando un cliente se conecta, un hijo es seleccionado por el sistema
 * para atenderlo. Si hay más clientes que procesos, las conexiones se encolan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../include/server_utils.h"
#include "../include/common.h"

#define BACKLOG 10          // Tamaño de la cola de conexiones en espera
#define MAX_CHILDREN 100    // Límite de procesos hijos

int server_fd;                          // Socket del servidor
pid_t child_pids[MAX_CHILDREN];         // Arreglo para almacenar PIDs de hijos
int num_children = 0;                   // Número real de hijos creados

pthread_t kill_monitor_thread;

// --- Lógica de atención individual (compartida entre hijos) ---
void handle_client_preforked(int client_fd) {
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    buffer[bytes_read] = '\0';
    printf("[PRE-FORKED] Request recibido:\n%s\n", buffer);

    // Parsear método y ruta solicitada
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    // Determinar el recurso a servir
    char resource[256];
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        strcpy(resource, "index.html");
    } else {
        strncpy(resource, path + 1, sizeof(resource) - 1);  // Quita el '/' inicial
        resource[sizeof(resource) - 1] = '\0';
    }

    // Construir la ruta completa al recurso
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);
    printf("[PRE-FORKED] Buscando archivo: %s\n", filepath);

    // Leer y enviar archivo si existe
    FILE *file = fopen(filepath, "r");
    char response[8192];

    if (!file) {
        // Responder con 404 si el recurso no se encuentra
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
        printf("[PRE-FORKED] %s no existe\n", filepath);
    } else {
        char file_content[4096];
        size_t read_bytes = fread(file_content, 1, sizeof(file_content) - 1, file);
        file_content[read_bytes] = '\0';
        fclose(file);

        // Respuesta 200 OK
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s\r\n", file_content);
    }

    // Enviar respuesta al cliente y cerrar conexión
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
    printf("[PRE-FORKED] Cliente atendido y desconectado.\n");
    fflush(stdout);  // Forzar salida inmediata
}

// --- Manejador de señal Ctrl+C para finalizar todos los hijos ---
void handle_sigint(int sig) {
    printf("\n[PRE-FORKED] Terminando procesos hijos...\n");

    // Enviar SIGTERM a cada hijo
    for (int i = 0; i < num_children; i++) {
        kill(child_pids[i], SIGTERM);
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < num_children; i++) {
        waitpid(child_pids[i], NULL, 0);
    }

    close(server_fd);
    printf("[PRE-FORKED] Servidor finalizado correctamente.\n");
    exit(0);
}

// Hilo que espera "KILL"
void* control_thread_preforked(void* arg) {
    char input[100];
    while (fgets(input, sizeof(input), stdin) != NULL) {
        if (strncmp(input, "KILL", 4) == 0) {
            printf("[PRE-FORKED] Comando KILL recibido. Cerrando servidor...\n");
            handle_sigint(SIGTERM);  // Ejecuta el mismo flujo que Ctrl+C
        }
    }
    return NULL;
}

// --- Función principal del modo Pre-Forked ---
void run_pre_forked(int k) {
    struct sockaddr_in server_addr;

    printf("[PRE-FORKED] Iniciando servidor con %d procesos hijos...\n", k);
    fflush(stdout);  // Forzar impresión inmediata

    // Crear socket TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Permitir reutilizar dirección/puerto
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Asociar socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Registrar manejador de Ctrl+C
    signal(SIGINT, handle_sigint);

    // Lanzar hilo de control para escuchar "KILL"
    if (pthread_create(&kill_monitor_thread, NULL, control_thread_preforked, NULL) != 0) {
        perror("pthread_create (KILL monitor)");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Crear K procesos hijos
    for (int i = 0; i < k && i < MAX_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Código del hijo
            struct sockaddr_in client_addr;
            socklen_t client_size = sizeof(client_addr);

            printf("[PRE-FORKED] Proceso hijo %d esperando conexiones...\n", getpid());
            fflush(stdout);

            // Cada hijo atiende indefinidamente
            while (1) {
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                printf("[PRE-FORKED] Proceso %d aceptó una conexión.\n", getpid());
                fflush(stdout);
                handle_client_preforked(client_fd);
            }

            // Nunca se alcanza
            close(server_fd);
            exit(EXIT_SUCCESS);
        } else {
            // Código del padre: almacenar PID del hijo
            child_pids[num_children++] = pid;
        }
    }

    // Proceso padre queda esperando señales (ej. Ctrl+C)
    printf("[PRE-FORKED] Proceso padre esperando señales (KILL para terminar).\n");
    fflush(stdout);

    while (1) pause();
}
