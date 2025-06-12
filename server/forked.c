/*
* Impl. servidor FORKED
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../include/server_utils.h"
#include "../include/common.h"

// CANTIDAD MÁXIMA DE CONEXIONES EN ESPERA (en la cola del sistema operativo)
#define BACKLOG 10

int server_fd_global_forked;

// Hilo que espera "KILL"
void* control_thread_forked(void* arg) {
    char input[100];
    while (1) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (strncmp(input, "KILL", 4) == 0) {
                printf("[FORKED] Comando KILL recibido. Cerrando servidor...\n");
                close(server_fd_global_forked);
                exit(0);  // Finaliza el proceso padre y detiene el servidor
            }
        }
    }
    return NULL;
}


// Prototipo de función para atender a un cliente
void handle_client_forked(int client_fd);

void run_forked() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[FORKED] Iniciando servidor FORKED en el puerto %d...\n", SERVER_PORT);

    // Crear socket del servidor (TCP/IP)
    if ((server_fd_global_forked = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Reutilizar dirección/puerto
    int opt = 1;
    if (setsockopt(server_fd_global_forked, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd_global_forked);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;            // IPv4
    server_addr.sin_port = htons(SERVER_PORT);   // Puerto definido en common.h
    server_addr.sin_addr.s_addr = INADDR_ANY;    // Acepta conexiones desde cualquier IP
    memset(&(server_addr.sin_zero), '\0', 8);    // Relleno requerido

    // Asociar socket con la dirección y puerto
    if (bind(server_fd_global_forked, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd_global_forked);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd_global_forked, BACKLOG) == -1) {
        perror("listen");
        close(server_fd_global_forked);
        exit(EXIT_FAILURE);
    }

    // Lanzar hilo para monitorear comando KILL
    pthread_t control_tid;
    if (pthread_create(&control_tid, NULL, control_thread_forked, NULL) != 0) {
        perror("pthread_create");
        close(server_fd_global_forked);
        exit(EXIT_FAILURE);
    }

    printf("[FORKED] Servidor esperando conexiones...\n");

    // Bucle infinito: aceptar y atender un cliente a la vez
    while (1) {
        client_fd = accept(server_fd_global_forked, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("[FORKED] accept cancelado o falló");
            break;
        }

        printf("[FORKED] Cliente conectado.\n");

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        } else if (pid == 0) {
            // Proceso hijo
            close(server_fd_global_forked); // El hijo no necesita el socket del servidor
            handle_client_forked(client_fd);
            close(client_fd);
            printf("[FORKED] Cliente desconectado.\n");
            exit(EXIT_SUCCESS);
        } else {
            // Proceso padre
            close(client_fd); // El padre no necesita el socket del cliente

            // Opcional: Recolectar procesos hijos terminados para evitar zombies
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }

    close(server_fd_global_forked);
    printf("[FORKED] Servidor finalizado.\n");
}

#define CHUNK_SIZE 4096

void handle_client_forked(int client_fd) {
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


