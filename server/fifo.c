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
    // SE LEE REQUEST Y RESPONDE AL CLIENTE
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    buffer[bytes_read] = '\0';
    printf("[FIFO] Request recibido:\n%s\n", buffer);

    // RESPONDE CON EL CONTENIDO DE INDEX.HTML
    char response[8192];
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/index.html", RESOURCE_DIR);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
    } else {
        char file_content[4096];
        fread(file_content, 1, sizeof(file_content), file);
        fclose(file);
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s\r\n", file_content);
    }

    send(client_fd, response, strlen(response), 0);
}
