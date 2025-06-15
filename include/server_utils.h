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
* Funciones auxiliares del servidor
*/

#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H
#define RESOURCE_DIR "../resources"
#define CHUNK_SIZE 15000000

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

void run_fifo();
void run_threaded();
void run_pre_threaded(int k);
void run_forked();
void run_pre_forked(int k);

static pthread_mutex_t writingMutex;

static long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000L);
}

static void handle_send(FILE *file, const char* resource, const int client_fd) {
    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Send headers
    char header[1024];
    printf("Recurso:%s\n", resource);
    strcmp("index.html",resource)==0 ?
        snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n", filesize)
    :
        snprintf(header, sizeof(header),
         "HTTP/1.1 200 OK\r\n"
         "Content-Type: application/octet-stream\r\n"
         "Content-Disposition: attachment; filename=\"%s\"\r\n"
         "Content-Length: %ld\r\n"
         "Connection: close\r\n\r\n",
         resource, filesize)
    ;

    send(client_fd, header, strlen(header), 0);

    char chunk[CHUNK_SIZE];
    size_t n;
    while ((n = fread(chunk, 1, CHUNK_SIZE, file)) > 0) {
        if (send(client_fd, chunk, n, 0) < 0) {
            perror("send");
            break;
        }
    }
}

static int handle_upload(const char* filepath, const int client_fd, const char* body_start, const int body_bytes, const int content_length) {
    FILE *out = fopen(filepath, "wb");
    if (!out) {
        perror("fopen");

        return 1;
    }

    // Escribir los primeros bytes que ya estaban en el buffer
    fwrite(body_start, 1, body_bytes, out);

    // Leer el resto del cuerpo
    int total_written = body_bytes;
    int remaining = content_length - body_bytes;
    char chunk[CHUNK_SIZE];

    while (remaining > 0) {
        printf("Remaining: %d\n", remaining);
        int to_read = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
        int n = recv(client_fd, chunk, to_read, 0);

        if (n <= 0) {
            printf("Se murio el cliente :(\nEliminando: %s\n",filepath);
            perror("recv (durante POST)");
            fclose(out);
            remove(filepath);  // Elimina archivo incompleto

            const char *error_response =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<html><body><h1>Error: conexión interrumpida o incompleta.</h1></body></html>\r\n";
            send(client_fd, error_response, strlen(error_response), 0);

            return 1;
        }

        fwrite(chunk, 1, n, out);
        total_written += n;
        remaining -= n;
    }

    fclose(out);

    if (total_written != content_length) {
        //verifica que se escribió exactamente lo esperado
        remove(filepath);
        printf("Se murio el cliente :(\nEliminando: %s\n",filepath);
        const char *error_response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>Error: datos incompletos.</h1></body></html>\r\n";
        send(client_fd, error_response, strlen(error_response), 0);
        pthread_mutex_unlock(&writingMutex);
        return 1;
    }

    const char *ok_response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h1>Archivo subido :)</h1></body></html>\r\n";
    send(client_fd, ok_response, strlen(ok_response), 0);

    printf("Paso algo\n");
    pthread_mutex_unlock(&writingMutex);
    return 0;
}



static void handle_client(const int client_fd) {
    char buffer[CHUNK_SIZE];
    int total_received = 0;
    int bytes_read;
    long start = get_time_ms();

    // Read until we get the full HTTP headers (ends in \r\n\r\n)
    while ((bytes_read = recv(client_fd, buffer + total_received, sizeof(buffer) - total_received - 1, 0)) > 0) {
        total_received += bytes_read;
        buffer[total_received] = '\0';
        printf("%s\n", buffer);
        if (strstr(buffer, "\r\n\r\n")) break;  // End of headers
    }

    if (bytes_read <= 0) {
        perror("recv");
        return;
    }

    // Extaer method, path, y Content-Length
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);
    printf("%s %s\n", method, path);

    char *content_length_str = strstr(buffer, "Content-Length: ");

    unsigned int content_length = 0;
    if (content_length_str) {
        sscanf(content_length_str, "Content-Length: %d", &content_length);
    }
    printf("Contenido: %u\n",content_length);

    char *body_start = strstr(buffer, "\r\n\r\n");
    body_start = strstr(body_start, "\r\n\r\n");
    if (!body_start) {
        printf("Malformed request\n");
        return;
    }
    body_start += 4;  // Skip past the header


    int header_size = body_start - buffer;
    int body_bytes = total_received - header_size;

    // Get filename from path (e.g., /uploaded.txt)
    char resource[256];
    strncpy(resource, path + 1, sizeof(resource) - 1);
    resource[sizeof(resource) - 1] = '\0';

    if (strcmp(path, "/") == 0 || strcmp(path, "/favicon.ico") == 0) {
        strcpy(resource, "index.html");
    } else {
        strncpy(resource, path + 1, sizeof(resource) - 1);
        resource[sizeof(resource) - 1] = '\0';
    }

    if (strcmp(method, "POST") == 0) pthread_mutex_lock(&writingMutex);
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", RESOURCE_DIR, resource);

    // Reject if file already exists
    FILE *file = fopen(filepath, "rb");

    if (file && strcmp(method, "POST") == 0) {
        printf("Ya existe: %s\n", filepath);
        const char *not_found_response =
            "HTTP/1.1 400 Bad request\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>400 El archivo ya existe :(</h1></body></html>\r\n";
        send(client_fd, not_found_response, strlen(not_found_response), 0);
        return;
    }
    pthread_mutex_unlock(&writingMutex);

    if (file && strcmp(method, "GET") == 0) handle_send(file, resource, client_fd);
    else if (!file && strcmp(method, "POST") == 0) {
        handle_upload(filepath, client_fd, body_start, body_bytes, content_length);

        printf("Mutex liberado\n");
    }
    else if (!file && strcmp(method, "GET") == 0) {
        printf("No existe: %s\n", filepath);
        const char *not_found_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Recurso no encontrado :(</h1></body></html>\r\n";
        send(client_fd, not_found_response, strlen(not_found_response), 0);
        return;
    }

    long end = get_time_ms();
    long TTA = end - start;
    printf("Request procesado en: %ld ms, %ld s\n", TTA, TTA/1000);

    if (file) fclose(file);

}
#endif // SERVER_UTILS_H
