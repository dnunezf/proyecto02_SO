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
        close(client_fd);
        // Cerrar conexión con el cliente actual
        printf("[FIFO] Cliente desconectado.\n");
    }

    // Nunca se llega aquí, pero por buena práctica
    close(server_fd_global);
    printf("[FIFO] Servidor finalizado.\n");
}

