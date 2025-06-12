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
 * Implementación del modo FIFO
 * Este modo atiende un solo cliente a la vez.
 * Las demás conexiones se encolan automáticamente por el sistema.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include "../include/server_utils.h"
#include "../include/common.h"

// CANTIDAD MÁXIMA DE CONEXIONES EN ESPERA (en la cola del sistema operativo)
#define BACKLOG 10

int server_fd_global; // Global para poder cerrarlo desde otro hilo

// Hilo que espera "KILL"
void *control_thread(void *arg) {
    char input[100];
    while (1) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (strncmp(input, "KILL", 4) == 0) {
                printf("[FIFO] Comando KILL recibido. Cerrando servidor...\n");
                close(server_fd_global);
                exit(0); // Termina todo el proceso
            }
        }
    }
    return NULL;
}

// Prototipo de función para atender a un cliente
void handle_client(int client_fd);

void run_fifo() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[FIFO] Iniciando servidor FIFO en el puerto %d...\n", SERVER_PORT);

    // Crear socket del servidor (TCP/IP)
    if ((server_fd_global = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Permitir reutilizar el puerto
    int opt = 1;
    if (setsockopt(server_fd_global, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;            // IPv4
    server_addr.sin_port = htons(SERVER_PORT);   // Puerto definido en common.h
    server_addr.sin_addr.s_addr = INADDR_ANY;    // Acepta conexiones desde cualquier IP
    memset(&(server_addr.sin_zero), '\0', 8);    // Relleno requerido

    // Asociar socket con la dirección y puerto
    if (bind(server_fd_global, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd_global, BACKLOG) == -1) {
        perror("listen");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    // Iniciar hilo de control
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, control_thread, NULL) != 0) {
        perror("pthread_create");
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    printf("[FIFO] Servidor esperando conexiones...\n");

    // Bucle infinito: aceptar y atender un cliente a la vez
    while (1) {
        // Esperar y aceptar una conexión (bloqueante)
        client_fd = accept(server_fd_global, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("[FIFO] accept cancelado o falló");
            break;
        }

        printf("[FIFO] Cliente conectado.\n");

        // Atender al cliente (recibir, procesar, responder)
        handle_client(client_fd);

        // Cerrar conexión con el cliente actual
        close(client_fd);
        printf("[FIFO] Cliente desconectado.\n");
    }

    // Nunca se llega aquí, pero por buena práctica
    close(server_fd_global);
    printf("[FIFO] Servidor finalizado.\n");
}

// Función que atiende una conexión específica
#define CHUNK_SIZE 4096

void handle_client(int client_fd) {
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    buffer[bytes_read] = '\0';
    printf("[FIFO] Request recibido:\n%s\n", buffer);

    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    char resource[256];
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        strcpy(resource, "index.html");
    } else {
        strncpy(resource, path + 1, sizeof(resource) - 1);
        resource[sizeof(resource) - 1] = '\0';
    }

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);

    printf("[FIFO] Buscando archivo: %s\n", filepath);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        printf("[FIFO] No existe: %s\n", filepath);
        const char *not_found_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n";
        send(client_fd, not_found_response, strlen(not_found_response), 0);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Send headers
    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n", filesize);
    send(client_fd, header, strlen(header), 0);

    // Send file in chunks
    char chunk[CHUNK_SIZE];
    size_t n;
    while ((n = fread(chunk, 1, CHUNK_SIZE, file)) > 0) {
        if (send(client_fd, chunk, n, 0) < 0) {
            perror("send");
            break;
        }
    }

    fclose(file);
}

