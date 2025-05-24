cmake_minimum_required(VERSION 3.10)
project(http_multiversion_server C)

set(CMAKE_C_STANDARD 11)

# Agrega la carpeta include como ruta válida de headers
include_directories(include)

# Archivos del servidor
set(SERVER_SOURCES
        server/main.c
        server/fifo.c
        server/threaded.c
        server/pre_threaded.c
        server/forked.c
        server/pre_forked.c
)

# Archivos del cliente
set(CLIENT_SOURCES
        Client/client.c
        Client/statistics.c
)

# Crea ejecutable para el servidor
add_executable(server_exec ${SERVER_SOURCES})

# Crea ejecutable para el cliente
add_executable(client_exec ${CLIENT_SOURCES})
