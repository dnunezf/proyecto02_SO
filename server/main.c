/*
* PROYECTO PROGRAMADO 02
* SISTEMAS OPERATIVOS
* ESTUDIANTES:
* Ignacio Rodríguez Ovares, 118940243
* David Núñez Franco, 119080008
* Sharon María Araya Ramírez, 402530296
* CREATED 24/05/2025
* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server_utils.h"
#include <pthread.h>

extern pthread_mutex_t writingMutex;

void print_usage(const char *prog_name) {
    printf("Uso: %s <modo> [cantidad]\n", prog_name);
    printf("Modo:\n");
    printf("  1 - FIFO\n");
    printf("  2 - Threaded\n");
    printf("  3 - Pre-Threaded <cantidad de hilos>\n");
    printf("  4 - Forked\n");
    printf("  5 - Pre-Forked <cantidad de procesos>\n");
}

int main(int argc, char *argv[]) {
    pthread_mutex_init(&writingMutex, NULL);
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int mode = atoi(argv[1]);

    switch (mode) {
        case 1:
            run_fifo();
        break;

        case 2:
            run_threaded();
        break;

        case 3:
            if (argc < 3) {
                fprintf(stderr, "Modo 3 (Pre-Threaded) requiere cantidad de hilos.\n");
                return EXIT_FAILURE;
            }
        run_pre_threaded(atoi(argv[2]));
        break;

        case 4:
            run_forked();
        break;

        case 5:
            if (argc < 3) {
                fprintf(stderr, "Modo 5 (Pre-Forked) requiere cantidad de procesos.\n");
                return EXIT_FAILURE;
            }
        run_pre_forked(atoi(argv[2]));
        break;

        default:
            fprintf(stderr, "Modo inválido.\n");

        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    pthread_mutex_destroy(&writingMutex);
    return EXIT_SUCCESS;
}
