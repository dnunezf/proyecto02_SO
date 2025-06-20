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
#define CHUNK_SIZE 3000000
#define _FILE_OFFSET_BITS 64

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

static char* formatos[19] = {
    ".html", ".htm", ".txt", ".json", ".xml", ".pdf",
    ".jpg", ".jpeg", ".png", ".gif", ".svg", ".webp", ".ico",
    ".mp3", ".wav", ".ogg", ".webm",
    ".mp4", ".ogv"
};

static char* formatosContent[19] = {
    "text/html", "text/html", "text/plain", "application/json", "application/xml","application/pdf",
    "image/jpeg", "image/jpeg", "image/png", "image/gif", "image/svg+xml", "image/webp", "image/x-icon",
    "audio/mpeg", "audio/wav", "audio/ogg", "audio/webm",
    "video/mp4", "video/ogg"
};

static char* fortunas[100] = {
    "La vida es corta, como el café que se acaba justo cuando más lo necesitás.",
    "Si todo está bajo control, es probable que te hayas quedado quieto demasiado tiempo.",
    "La suerte favorece al que ya tenía un plan de respaldo.",
    "Todo pasa por algo. A veces ese algo sos vos.",
    "La ironía de la vida es que nadie sale vivo de ella.",
    "Lo urgente rara vez es importante, pero igual te va a perseguir.",
    "No todos los que vagan están perdidos. Algunos simplemente no quieren ser encontrados.",
    "La paciencia es una virtud… que se agota después del tercer intento.",
    "El silencio no siempre es sabio. A veces es solo WiFi caído.",
    "Nada es imposible, excepto encontrar el extremo correcto del tape.",
    "La sabiduría viene con la experiencia. La experiencia viene con los errores.",
    "No hagas hoy lo que puedas posponer para siempre.",
    "Hay dos tipos de personas: las que entienden binario y las que no.",
    "Todo gran logro comienza con una idea… y una taza de café.",
    "El que madruga… probablemente se quedó dormido temprano.",
    "La esperanza es lo último que se pierde, pero a veces se esconde bien.",
    "Si te caés siete veces, revisá el suelo. Capaz está resbaloso.",
    "Los errores son pruebas de que estás intentando. O que no leíste las instrucciones.",
    "El que ríe último, probablemente no entendió el chiste.",
    "Antes de encontrar el camino, uno suele perderse varias veces.",
    "Las mejores cosas de la vida no son cosas.",
    "La lógica te lleva de A a B. La imaginación, a errores más interesantes.",
    "El futuro es incierto. Aprovechá el WiFi mientras dure.",
    "Tu zona de confort es linda, pero no crece nada ahí.",
    "La mente es como un paracaídas: funciona mejor cuando se abre.",
    "Cada problema trae una oportunidad. A veces solo trae estrés.",
    "El universo conspira a tu favor… cuando no tiene nada mejor que hacer.",
    "No todos los héroes usan capa. Algunos ni se levantan de la cama.",
    "Confundirse es parte del camino. Quedarse ahí es decoración.",
    "Todo es temporal. Incluso el 'para siempre'.",
    "Aceptá el caos. Es más organizado que vos.",
    "La motivación es pasajera. El hábito se queda a vivir.",
    "Si vas a fallar, hacelo con estilo.",
    "No sabés lo fuerte que sos hasta que levantás el mueble sin ayuda.",
    "El miedo al cambio es real. Pero también lo es el aburrimiento.",
    "Los días difíciles también terminan. Aunque se tomen su tiempo.",
    "Podés con más de lo que creés. Menos con los lunes.",
    "La suerte no golpea dos veces… pero a veces manda mensaje.",
    "Cuando no sabés qué hacer, respirá. Y fingí que estás pensando.",
    "El tiempo no cura todo. Pero lo distrae un poco.",
    "No busques el sentido de la vida en Google.",
    "La creatividad nace del aburrimiento. O de no tener WiFi.",
    "Más vale tarde que excusa.",
    "Pensar demasiado es una forma elegante de procrastinar.",
    "No confundas paz con desconexión.",
    "Todo lo que sube… probablemente dejó algo olvidado abajo.",
    "Los planes son geniales hasta que aparece la realidad.",
    "La vida no es una línea recta. Es más bien un doodle en una servilleta.",
    "Elegí tus batallas. Algunas no valen ni el sarcasmo.",
    "A veces no hay moraleja. Solo caos con sonido de fondo.",
    "Vivir al límite no significa dejar el microondas sin apagar.",
    "No todo necesita una respuesta. A veces solo necesita silencio.",
    "El perfeccionismo es una trampa disfrazada de esfuerzo.",
    "La honestidad brutal a veces es solo brutal.",
    "La rutina mata más sueños que el fracaso.",
    "Tu peor enemigo te conoce bien… porque sos vos con sueño.",
    "La realidad supera a la ficción. Pero con menos presupuesto.",
    "La adultez es descubrir que nadie sabe realmente lo que hace.",
    "Aprendé a soltar. Incluso si el archivo no se guarda.",
    "El multitasking es la manera profesional de hacer muchas cosas mal a la vez.",
    "El confort es lindo… hasta que se convierte en estancamiento.",
    "A veces lo correcto no se siente bien. Pero igual lo es.",
    "La paz interior cuesta más que una suscripción premium.",
    "Los finales felices existen. Pero no suelen ser gratis.",
    "No todo cambio es mejora, pero toda mejora requiere cambio.",
    "El caos es solo orden en idioma extranjero.",
    "La madurez es saber cuándo callarte aunque tengas razón.",
    "A veces el camino se construye mientras lo caminás.",
    "La nostalgia es el filtro sepia de la memoria.",
    "El éxito sin propósito es solo acumulación.",
    "No hay señal más clara que el instinto ignorado.",
    "Aprender duele. Pero ignorar duele más tiempo.",
    "Los límites también son una forma de amor propio.",
    "Hay decisiones que se sienten mal porque son correctas.",
    "Ser fuerte no significa no quebrarse. Significa reconstruirse.",
    "Agradecé incluso lo que no salió como querías.",
    "El descanso también es productivo.",
    "La intuición es la lógica que todavía no entiende tu cabeza.",
    "No es pereza si estás procesando la existencia.",
    "Algunos caminos son círculos disfrazados.",
    "Lo que te molesta, te enseña.",
    "Las metas no se alcanzan mirando la barra de progreso.",
    "El fracaso también es una dirección.",
    "La claridad llega después del caos, no antes.",
    "La aceptación es la cura que no venden.",
    "No todo pensamiento necesita ser dicho.",
    "A veces no estás perdiendo el tiempo. Estás recuperando el alma.",
    "Si no sabés qué hacer, hacé una pausa.",
    "No podés controlar todo. Pero podés respirar.",
    "La autocompasión no es debilidad. Es humanidad.",
    "Compararte solo te aleja de vos mismo.",
    "El que se va sin ser echado… probablemente vuelva por la ventana.",
    "Algunos comienzos parecen finales.",
    "Todo lleva tiempo. Incluso entender eso.",
    "No te definís por tu peor día.",
    "Revisá tus pensamientos como si fueran notificaciones: no todos merecen tu atención.",
    "Cada tanto, borrá la caché emocional.",
    "Ser feliz no siempre es estar contento. A veces es estar en paz.",
    "No estás roto. Estás en proceso.",
    "Las respuestas que buscás a veces llegan en silencio."
};

static const char* obtenerExtension(const char* filepath) {
    char* punto = strrchr(filepath, '.');
    if (!punto || punto == filepath) {
        return NULL; // No tiene :(
    }
    return punto;
}

static char* reconstruirFilepath(const char* filepath) {
    const char* media_prefix = "../resources/media/";
    const char* new_prefix = "../resources/";

    size_t media_len = strlen(media_prefix);
    size_t new_prefix_len = strlen(new_prefix);

    if (strncmp(filepath, media_prefix, media_len) != 0) {
        return NULL; // Not a /media path
    }

    const char* subpath = filepath + media_len;
    size_t total_len = new_prefix_len + strlen(subpath) + 1;
    char* result = malloc(total_len); //Se tiene que matar, importante
    if (!result) return NULL;

    strcpy(result, new_prefix);
    strcat(result, subpath);

    return result;
}

static long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000L);
}



static off_t get_file_size(FILE *file) {
    off_t current_pos = ftello(file);
    fseeko(file, 0, SEEK_END);
    off_t size = ftello(file);
    fseeko(file, current_pos, SEEK_SET);
    return size;
}

static void handle_send(FILE *file, const char* resource, const int client_fd, int extNum) {
    off_t filesize = get_file_size(file);
    printf("File size: %lld\n", (long long)filesize);

    printf("EXT: %d\n", extNum);
    printf("Recurso: %s\n", resource);

    char header[1024];

    if (strcmp("index.html", resource) == 0) {
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %lld\r\n"
            "Connection: close\r\n\r\n",
            (long long)filesize);
    } else if (extNum == -1) {
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Disposition: attachment; filename=\"%s\"\r\n"
            "Content-Length: %lld\r\n"
            "Connection: close\r\n\r\n",
            resource,
            (long long)filesize);
    } else {
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Disposition: inline; filename=\"%s\"\r\n"
            "Content-Length: %lld\r\n"
            "Connection: close\r\n\r\n",
            formatosContent[extNum],
            resource,
            (long long)filesize);
    }

    send(client_fd, header, strlen(header), 0);

    char chunk[CHUNK_SIZE];
    size_t n;
    size_t read = 0;
    while ((n = fread(chunk, 1, sizeof(chunk), file)) > 0) {
        if (send(client_fd, chunk, n, 0) < 0) {
            perror("send");
            break;
        }
        read += n;
        printf("%lu de %lu enviados. %fp\n", filesize, read, (float)(read* 100)/filesize);
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
            printf("Se murio el cliente :(\nEliminando: %s Razon:Interrupcion\n" ,filepath);
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
        //verifica que se escribió exactamente lo esperado (cantidad)
        remove(filepath);
        printf("Se murio el cliente :(\nEliminando: %s Razon:incompletos\n",filepath);
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

    // Leer hasta \r\n\r\n (fin de cabecera)
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

    char* filepathPuro = reconstruirFilepath(filepath);
    FILE *file = !filepathPuro ? fopen(filepath, "rb") : fopen(filepathPuro, "rb");

    int extNum = -1;
    if (filepathPuro) {

        printf("Extensionnn: %s %s \n", filepathPuro, resource);

        char* resource2 = strrchr(resource, '/');

        strncpy(resource, resource2, sizeof(resource) - 1);

        char* extension = strrchr(resource, '.');
        if (extension)
        for (int i = 0; i < 19; i++) {
            if (strcmp(extension, formatos[i]) == 0) {
                extNum = i;
                break;
            }
        }
        printf("Extnum: %d, ext: %s\n", extNum, extension);
    }

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

    if (file && strcmp(method, "GET") == 0) {
        printf("enviando algo\n");
        handle_send(file, resource, client_fd, extNum);
    }
    else if (!file && strcmp(method, "POST") == 0) {
        handle_upload(filepath, client_fd, body_start, body_bytes, content_length);

        printf("Mutex liberado\n");
    }
    else if (!file && strcmp(method, "GET") == 0) {

        char* header[1024];
        srand(time(NULL));
        unsigned int fortuna = rand()%100;
        printf("No existe: %s\n", filepath);

        snprintf(header, sizeof(header),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>"
        "<head><meta charset='UTF-8'>"
        "<style>"
        "body {background-color: powderblue;}"
        "h1 {color: blue; text-align: center;} "
        "summary {color: red; text-align: center;}"
        "details {text-align: center;}"
        "img {display: block; margin: auto;}"
        "</style>"
        "</head>"
        "<body><h1>404 Recurso no encontrado :(</h1>"
        "<img src='https://cdn3.emoji.gg/emojis/2426-minecraft-fox-spin.gif' alt='Zorro fiumm'>"
        "<details>"
        "<summary class>Ver mi fortuna</summary>"
        "%s"
        "</details>"
        "</body>"
        "</html>\r\n",
        fortunas[fortuna]);


        send(client_fd, header, strlen(header), 0);
        return;
    }

    long end = get_time_ms();
    long TTA = end - start;
    printf("Request procesado en: %ld ms, %ld s\n", TTA, TTA/1000);

    if (file) fclose(file);

}
#endif // SERVER_UTILS_H
