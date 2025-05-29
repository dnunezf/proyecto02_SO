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
 * Implementación del modo PRE-THREADED
 * Se crean K hilos al inicio que permanecen vivos.
 * Cada hilo espera clientes en una cola compartida.
 * Las conexiones nuevas se encolan si todos los hilos están ocupados.
 */

/*
 * Nota: Para simular múltiples hilos concurrentes,
 * for i in {1..10}; do curl http://localhost:8080/ & done
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/server_utils.h"
#include "../include/common.h"

#define BACKLOG 10         // Cantidad máxima de conexiones en espera (cola del SO)
#define MAX_QUEUE 100      // Tamaño máximo de la cola de conexiones personalizada

// Cola circular para almacenar conexiones entrantes
int connection_queue[MAX_QUEUE];
int queue_start = 0;
int queue_end = 0;
int queue_count = 0;

// Variables para sincronización entre hilos
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

// Agrega una conexión al final de la cola (bloquea si la cola está llena)
void enqueue(int client_fd) {
    pthread_mutex_lock(&queue_mutex);
    while (queue_count == MAX_QUEUE) {
        // Espera si la cola está llena (opcional, dependiendo del diseño)
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }

    connection_queue[queue_end] = client_fd;
    queue_end = (queue_end + 1) % MAX_QUEUE;
    queue_count++;

    pthread_cond_signal(&queue_not_empty);  // Notifica a los hilos que hay trabajo
    pthread_mutex_unlock(&queue_mutex);
}

// Toma una conexión de la cola para ser procesada por un hilo (bloquea si está vacía)
int dequeue() {
    pthread_mutex_lock(&queue_mutex);
    while (queue_count == 0) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }

    int client_fd = connection_queue[queue_start];
    queue_start = (queue_start + 1) % MAX_QUEUE;
    queue_count--;

    pthread_mutex_unlock(&queue_mutex);
    return client_fd;
}

// Función que ejecutan los hilos trabajadores (esperan conexiones y las atienden)
void* worker_thread(void* arg) {
    while (1) {
        int client_fd = dequeue();  // Espera una conexión

        printf("[PRE-THREADED] Atendiendo cliente en hilo %lu\n", pthread_self());

        // Reutiliza la lógica del modo FIFO para procesar la solicitud
        char buffer[4096];
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            perror("recv");
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0';

        // Extraer ruta solicitada del request HTTP
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

        FILE *file = fopen(filepath, "r");
        char response[8192];

        if (!file) {
            // Respuesta personalizada para error 404
            snprintf(response, sizeof(response),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/html\r\n\r\n"
                     "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n");
        } else {
            // Si se encuentra el archivo, se envía al cliente
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
        close(client_fd);
    }

    return NULL;
}

// Función principal para levantar el servidor en modo Pre-Threaded
void run_pre_threaded(int k) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    printf("[PRE-THREADED] Iniciando servidor con %d hilos...\n", k);

    // Crear socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección y puerto del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Crear pool de hilos
    pthread_t threads[k];
    for (int i = 0; i < k; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    printf("[PRE-THREADED] Servidor esperando conexiones...\n");

    // Aceptar conexiones y colocarlas en la cola
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        enqueue(client_fd);  // Agrega la conexión a la cola
    }

    close(server_fd);
}
