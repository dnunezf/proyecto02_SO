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

void run_fifo();
void run_threaded();
void run_pre_threaded(int k);
void run_forked();
void run_pre_forked(int k);

#endif // SERVER_UTILS_H
