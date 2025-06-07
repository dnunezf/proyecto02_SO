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
#include "../include/server_utils.h"
#include "../include/common.h"

// CANTIDAD MÁXIMA DE CONEXIONES EN ESPERA (en la cola del sistema operativo)
#define BACKLOG 10

// Prototipo de función para atender a un cliente
void handle_client(int client_fd);

void run_fifo() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[FIFO] Iniciando servidor FIFO en el puerto %d...\n", SERVER_PORT);

    // Crear socket del servidor (TCP/IP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;            // IPv4
    server_addr.sin_port = htons(SERVER_PORT);   // Puerto definido en common.h
    server_addr.sin_addr.s_addr = INADDR_ANY;    // Acepta conexiones desde cualquier IP
    memset(&(server_addr.sin_zero), '\0', 8);    // Relleno requerido

    // Asociar socket con la dirección y puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[FIFO] Servidor esperando conexiones...\n");

    // Bucle infinito: aceptar y atender un cliente a la vez
    while (1) {
        // Esperar y aceptar una conexión (bloqueante)
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("accept");
            continue;  // Si falla, intenta la siguiente conexión
        }

        printf("[FIFO] Cliente conectado.\n");

        // Atender al cliente (recibir, procesar, responder)
        handle_client(client_fd);

        // Cerrar conexión con el cliente actual
        close(client_fd);
        printf("[FIFO] Cliente desconectado.\n");
    }

    // Nunca se llega aquí, pero por buena práctica
    close(server_fd);
}

// Función que atiende una conexión específica
void handle_client(int client_fd) {
    char buffer[4096];  // Buffer para almacenar el request HTTP
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    buffer[bytes_read] = '\0';  // Asegura que el string esté terminado
    printf("[FIFO] Request recibido:\n%s\n", buffer);

    // Parseo de la primera línea del request: método y ruta
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    // Determinar el archivo a servir
    char resource[256];
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        // Si es la raíz o el ícono del navegador, servir index.html
        strcpy(resource, "index.html");
    } else {
        // Quitar el slash inicial para obtener el nombre del archivo
        strncpy(resource, path + 1, sizeof(resource) - 1);
        resource[sizeof(resource) - 1] = '\0';
    }

    // Construir la ruta completa al archivo solicitado
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);

    printf("[FIFO] Buscando archivo: %s\n", filepath);

    FILE *file = fopen(filepath, "r");

    char response[8192];  // Buffer para almacenar la respuesta HTTP
    if (!file) {
        // Si el archivo no existe, enviar respuesta 404 personalizada
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
        printf("%s no existe\n", filepath);
    } else {
        // Leer contenido del archivo y formar respuesta HTTP 200
        char file_content[4096];
        size_t read_bytes = fread(file_content, 1, sizeof(file_content) - 1, file);
        file_content[read_bytes] = '\0';
        fclose(file);

        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s\r\n", file_content);
    }

    // Enviar respuesta al cliente
    send(client_fd, response, strlen(response), 0);
}
