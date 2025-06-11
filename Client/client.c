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
* Impl. cliente
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUFFER_SIZE 4096
#define MAX_STR_LEN 100
#include <sys/time.h>

unsigned int THREADS, NUM_REQUESTS, SERVER_PORT;
char FILE_NAME[MAX_STR_LEN];
char SERVER_IP[MAX_STR_LEN];



long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000L);
}

// Definición del nodo
typedef struct Nodo {
    long valor;
    struct Nodo* siguiente;
} Nodo;

// Definición de la lista
typedef struct Lista {
    Nodo* cabeza;
} Lista;

// Función para inicializar la lista
void inicializarLista(Lista* lista) {
    lista->cabeza = NULL;
}

// Función para insertar al inicio
void insertarInicio(Lista* lista, long valor) {
    Nodo* nuevo = (Nodo*)malloc(sizeof(Nodo));
    nuevo->valor = valor;
    nuevo->siguiente = lista->cabeza;
    lista->cabeza = nuevo;
}

// Función para imprimir la lista
void imprimirLista(const Lista* lista) {
    Nodo* actual = lista->cabeza;
    while (actual != NULL) {
        printf("%ld -> ", actual->valor);
        actual = actual->siguiente;
    }
    printf("NULL\n");
}

// Función para liberar la memoria
void liberarLista(Lista* lista) {
    Nodo* actual = lista->cabeza;
    while (actual != NULL) {
        Nodo* temp = actual;
        actual = actual->siguiente;
        free(temp);
    }
    lista->cabeza = NULL;
}

// Función para calcular la media
long calcularMedia(const Lista* lista) {
    if (lista->cabeza == NULL) return 0;

    long suma = 0;
    long contador = 0;
    Nodo* actual = lista->cabeza;

    while (actual != NULL) {
        suma += actual->valor;
        contador++;
        actual = actual->siguiente;
    }

    return suma / contador;
}

// Función para calcular la varianza
long calcularVarianza(const Lista* lista) {
    if (lista->cabeza == NULL) return 0;

    const long media = calcularMedia(lista);
    long sumaDiferenciasCuadrado = 0;
    int contador = 0;
    const Nodo* actual = lista->cabeza;

    while (actual != NULL) {
        const long diferencia = actual->valor - media;
        sumaDiferenciasCuadrado += diferencia * diferencia;
        contador += 1;
        actual = actual->siguiente;
    }

    return sumaDiferenciasCuadrado / contador;
}



// Parsing de argumentos
void parse_args(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <THREADS> <NUM_REQUESTS> <FILE_NAME> <SERVER_IP> <SERVER_PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    THREADS = (unsigned int)strtoul(argv[1], NULL, 10);
    NUM_REQUESTS = (unsigned int)strtoul(argv[2], NULL, 10);

    strncpy(FILE_NAME, argv[3], MAX_STR_LEN - 1);
    FILE_NAME[MAX_STR_LEN - 1] = '\0';

    strncpy(SERVER_IP, argv[4], MAX_STR_LEN - 1);
    SERVER_IP[MAX_STR_LEN - 1] = '\0';

    SERVER_PORT = (unsigned int)strtoul(argv[5], NULL, 10);

}

Lista tiemposAtencion;
Lista tiemposEspera;
pthread_mutex_t mutex;
unsigned long long BytesTotal = 0;
long reqNum = 0;
// Procedimiento que ejecutará cada hilo
void* http_thread_routine(void* arg) {

    long espera[2];
    long atencion[2];

    printf("Thread corriendo: %ld.\n", (long)arg);
    for (unsigned int i = 0; i < NUM_REQUESTS; i++) {

        int sock_fd;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];

        // Crear socket
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            pthread_exit(NULL);
        }

        // Configurar dirección del servidor
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        memset(&(server_addr.sin_zero), '\0', 8);

        // Conectarse al servidor
        espera[0] = get_time_ms();
        if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(sock_fd);
            continue;
        }
        espera[1] = get_time_ms();
        atencion[0] = get_time_ms();
        // Enviar petición HTTP
        char http_request[512]; // Ajusta el tamaño según lo necesites
        snprintf(http_request, sizeof(http_request),
                 "GET /%s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", FILE_NAME);

        if (send(sock_fd, http_request, strlen(http_request), 0) == -1) {
            perror("send");
            close(sock_fd);
            continue;
        }


        // Leer respuesta
        int bytes_received;
        int header_parsed = 0;
        char *body_start = NULL;
        unsigned long long contBytes = 0;

        while ((bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            contBytes += bytes_received;
            buffer[bytes_received] = '\0';
            /*if (!header_parsed) {
                body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4; // Skip past headers
                    printf("%s", body_start); // Print body starting from here
                    header_parsed = 1;
                }
                // If headers not yet complete, skip printing
            } else {
                printf("%s", buffer); // Print rest of body
            }*/
        }
        atencion[1] = get_time_ms();

        pthread_mutex_lock(&mutex);
        insertarInicio(&tiemposAtencion, atencion[1] - atencion[0]);
        insertarInicio(&tiemposEspera, espera[1] - espera[0]);
        BytesTotal += contBytes;
        reqNum++;
        pthread_mutex_unlock(&mutex);
        close(sock_fd);
    }


    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    pthread_t threads[THREADS];

    // Crear THREADS hilos
    pthread_mutex_init(&mutex, NULL);
    long start = get_time_ms();
    inicializarLista(&tiemposAtencion);
    inicializarLista(&tiemposEspera);
    for (unsigned int i = 0; i < THREADS; i++) {
        if (pthread_create(&threads[i], NULL, http_thread_routine, (void*)i) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a que todos los hilos terminen
    for (unsigned int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    long end = get_time_ms();
    long TTA = end - start;

    unsigned long TEM = calcularMedia(&tiemposEspera);
    unsigned long TEV = calcularVarianza(&tiemposEspera);
    unsigned long TAM = calcularMedia(&tiemposAtencion);
    unsigned long TAV = calcularVarianza(&tiemposAtencion);
    printf("Tiempo total de atencion: %ld ms, %ld s\n", TTA, TTA/1000);
    printf("Tiempo de espera promedio: %ld ms, %ld s\n", TEM, TEM/1000 );
    printf("Varianza de tiempos de espera: %ld ms, %ld s\n",TEV, TEV/1000 );
    printf("Tiempo de atencion promedio: %ld ms, %ld s\n",TAM, TAM/1000 );
    printf("Varianza de tiempo de atencion: %ld ms, %ld s\n",TAV, TAV/1000);
    printf("Total de bytes recibidos: %lu B, %lu MB\n", BytesTotal, BytesTotal/1048576);
    printf("Numero de requests: %ld\n", reqNum);
    printf("Numero de perdidas: %ld\n", (THREADS * NUM_REQUESTS) - reqNum);

    pthread_mutex_destroy(&mutex);
    return 0;
}


