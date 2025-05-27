/*
* PROYECTO PROGRAMADO 02
* SISTEMAS OPERATIVOS
* ESTUDIANTES:
* Ignacio Rodríguez Ovares, 118940243
* David Núñez Franco, 119080008
* Sharon María Araya Ramírez, 402530296
* CREATED 24/05/2025
* */

/*
 * Impl. servidor FIFO
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/server_utils.h"
#include "../include/common.h"

// CANTIDAD MAX. DE CONEXIONES EN EXPERA
#define BACKLOG 10

void handle_client(int client_fd);

void run_fifo() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[FIFO] Iniciando servidor FIFO en el puerto %d...\n", SERVER_PORT);

    // SE CREA EL SOCKET
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // CONFIGURAR DIRECCION
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // ASOCIAR SOCKET
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // ESCUCHAR CONEXIONES
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[FIFO] Servidor esperando conexiones...\n");

    while (1) {
        // ACEPTA CONEXION (BLOQUEANTE)
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("[FIFO] Cliente conectado.\n");

        // ATIENDE AL CLIENTE
        handle_client(client_fd);

        close(client_fd);
        printf("[FIFO] Cliente desconectado.\n");
    }

    close(server_fd);
}

void handle_client(int client_fd) {
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    buffer[bytes_read] = '\0';
    printf("[FIFO] Request recibido:\n%s\n", buffer);

    // EXTRAER PRIMERA LÍNEA (ej. "GET / HTTP/1.1")
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    // VERIFICAR Y LIMPIAR PATH
    char resource[256];
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        strcpy(resource, "index.html");
    } else {
        // QUITAR EL SLASH INICIAL
        strncpy(resource, path + 1, sizeof(resource) - 1);
        resource[sizeof(resource) - 1] = '\0';
    }

    // CONSTRUIR RUTA COMPLETA AL ARCHIVO
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);

    printf("[FIFO] Buscando archivo: %s\n", filepath);

    FILE *file = fopen(filepath, "r");

    char response[8192];
    if (!file) {
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
    } else {
        char file_content[4096];
        size_t read_bytes = fread(file_content, 1, sizeof(file_content) - 1, file);
        file_content[read_bytes] = '\0';
        fclose(file);

        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s\r\n", file_content);
    }

    send(client_fd, response, strlen(response), 0);
}
