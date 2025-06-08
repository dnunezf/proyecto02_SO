/*
* Impl. servidor FORKED
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/server_utils.h"
#include "../include/common.h"

// CANTIDAD MÁXIMA DE CONEXIONES EN ESPERA (en la cola del sistema operativo)
#define BACKLOG 10

// Declarar server_fd global para que pueda cerrarse desde el handler ---
int server_fd;

// Manejador de señal SIGINT ---
void handle_sigint_forked(int sig) {
    printf("\n[FORKED] Finalizando servidor por señal SIGINT...\n");
    close(server_fd);
    exit(0);
}

// Prototipo de función para atender a un cliente
void handle_client_forked(int client_fd);

void run_forked() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[FORKED] Iniciando servidor FORKED en el puerto %d...\n", SERVER_PORT);

    // registrar handler de Ctrl+C
    signal(SIGINT, handle_sigint_forked);

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

    printf("[FORKED] Servidor esperando conexiones...\n");

    // Bucle infinito: aceptar y atender un cliente a la vez
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("[FORKED] Cliente conectado.\n");

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        } else if (pid == 0) {
            // Proceso hijo
            close(server_fd); // El hijo no necesita el socket del servidor
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

    close(server_fd);
}
