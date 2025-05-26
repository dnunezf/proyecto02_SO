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
 * Definiciones comunes (constantes, estructuras, funciones auxiliares)
*/

#ifndef COMMON_H
#define COMMON_H

#include <time.h>

#define SERVER_PORT 8080
#define RESOURCE_DIR "./resources"

//Tiempo actual en segundos
static time_t getCurrentTime() {
    time_t rawtime;
    time ( &rawtime );
    return rawtime;
}

//Tiempo actual con formato
static struct  tm* getTimestamp() {
    const time_t time = getCurrentTime();
    return localtime ( &time);
}

#endif //COMMON_H
