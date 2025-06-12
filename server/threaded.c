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
 * Implementación del modo THREADED
 * Cada vez que un cliente se conecta, se crea un nuevo hilo (light-weight).
 * El hilo atiende al cliente y luego termina (muere).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "../include/server_utils.h"
#include "../include/common.h"

#define BACKLOG 10  // Máximo número de conexiones en espera

int server_fd_global_threaded;

// Hilo que espera "KILL"
void* control_thread_threaded(void* arg) {
    char input[100];
    while (1) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (strncmp(input, "KILL", 4) == 0) {
                printf("[THREADED] Comando KILL recibido. Cerrando servidor...\n");
                close(server_fd_global_threaded);
                exit(0); // Terminar el proceso completo
            }
        }
    }
    return NULL;
}


/*
 * Función ejecutada por cada hilo que atiende un cliente.
 * El hilo recibe el descriptor del cliente, procesa el request
 * y envía la respuesta. Luego, finaliza.
 */
void* handle_client_thread(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);  // Libera memoria reservada dinámicamente para el descriptor

    printf("[THREADED] Cliente conectado. Hilo: %lu\n", pthread_self());

    // --- Leer la petición HTTP del cliente ---
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        perror("recv");
        close(client_fd);
        return NULL;
    }

    buffer[bytes_read] = '\0';  // Asegura terminación de string
    printf("[THREADED] Request recibido:\n%s\n", buffer);

    // --- Extraer método y ruta del request ---
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    // --- Determinar recurso solicitado ---
    char resource[256];
    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        strcpy(resource, "index.html");  // Recurso por defecto
    } else {
        strncpy(resource, path + 1, sizeof(resource) - 1);  // Quita el '/'
        resource[sizeof(resource) - 1] = '\0';
    }

    // --- Construir la ruta absoluta al recurso ---
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);
    printf("[THREADED] Buscando archivo: %s\n", filepath);

    // --- Leer archivo solicitado y construir respuesta HTTP ---
    FILE *file = fopen(filepath, "r");
    char response[8192];

    if (!file) {
        // Si no se encuentra el recurso, responder con error 404
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
        printf("[THREADED] %s no existe\n", filepath);
    } else {
        // Leer contenido del archivo
        char file_content[4096];
        size_t read_bytes = fread(file_content, 1, sizeof(file_content) - 1, file);
        file_content[read_bytes] = '\0';
        fclose(file);

        // Construir respuesta HTTP 200 OK
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "%s\r\n", file_content);
    }

    // --- Enviar respuesta al cliente ---
    send(client_fd, response, strlen(response), 0);
    close(client_fd);  // Cerrar conexión con cliente
    printf("[THREADED] Cliente desconectado. Hilo %lu finalizado.\n", pthread_self());

    return NULL;
}

/*
 * Función principal del modo threaded.
 * Crea un socket del servidor y entra en un bucle aceptando conexiones.
 * Por cada conexión aceptada, se crea un hilo independiente.
 */
void run_threaded() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[THREADED] Iniciando servidor THREADED en el puerto %d...\n", SERVER_PORT);

    // --- Crear socket TCP/IP ---
    if ((server_fd_global_threaded = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // --- Reutilizar dirección/puerto (evitar “address already in use”) ---
    int opt = 1;
    if (setsockopt(server_fd_global_threaded, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd_global_threaded);
        exit(EXIT_FAILURE);
    }

    // --- Configurar dirección del servidor ---
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // --- Asociar socket a la dirección ---
    if (bind(server_fd_global_threaded, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd_global_threaded);
        exit(EXIT_FAILURE);
    }

    // --- Escuchar conexiones entrantes ---
    if (listen(server_fd_global_threaded, BACKLOG) == -1) {
        perror("listen");
        close(server_fd_global_threaded);
        exit(EXIT_FAILURE);
    }

    // Lanzar hilo de control para capturar "KILL"
    pthread_t control_tid;
    if (pthread_create(&control_tid, NULL, control_thread_threaded, NULL) != 0) {
        perror("pthread_create (control_thread_threaded)");
        close(server_fd_global_threaded);
        exit(EXIT_FAILURE);
    }


    printf("[THREADED] Servidor esperando conexiones...\n");

    // --- Bucle principal ---
    while (1) {
        // Aceptar conexión de un cliente
        client_fd = accept(server_fd_global_threaded, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("[THREADED] accept cancelado o falló");
            break;
        }

        // Reservar memoria dinámica para pasar el descriptor al hilo
        int* client_ptr = malloc(sizeof(int));
        if (!client_ptr) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *client_ptr = client_fd;

        // Crear hilo para atender al cliente
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_thread, client_ptr) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_ptr);
            continue;
        }

        // Separar el hilo (no necesita join, se libera automáticamente)
        pthread_detach(thread_id);
    }

    // Nunca se alcanza, pero se incluye por buenas prácticas
    close(server_fd_global_threaded);
    printf("[THREADED] Servidor finalizado.\n");
}
