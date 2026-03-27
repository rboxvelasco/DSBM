#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define BAUD_RATE   B9600

#include <stdio.h>
#include <time.h>
#include "lib.h"
#include "colors.h"

Font Font5x7_struct = { Font5x7, 5, 7 };



// --- Variables globals ----------------------------------------
static int  fd          = -1;
static volatile int running = 1;
// -------------------------------------------------------------


int open_serial(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "[ERROR] No s'ha pogut obrir %s: %s\n", port, strerror(errno));
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "[ERROR] tcgetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    // Baud rate
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    // 8N1: 8 bits de dades, sense paritat, 1 stop bit
    tty.c_cflag &= ~PARENB;         // Sense paritat
    tty.c_cflag &= ~CSTOPB;         // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |=  CS8;            // 8 bits de dades

    // Sense control de flux hardware
    tty.c_cflag &= ~CRTSCTS;

    // Habilita recepció, ignora modem controls
    tty.c_cflag |= CREAD | CLOCAL;

    // Mode raw (sense processament especial)
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);        // Sense control de flux software
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK |
                     ISTRIP | INLCR  | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;                          // Sense processament de sortida

    // Timeout de lectura: 1 segon
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;   // 10 * 100ms = 1 segon

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "[ERROR] tcsetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    printf("[OK] Port obert: %s @ 9600 baud 8N1\n", port);
    return fd;
}


// --- Thread de recepció ---------------------------------------
void *rx_thread(void *arg) {
    (void)arg;
    char buf[64];

    while (running) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("[RX] ");
            for (int i = 0; i < n; i++) {
                unsigned char c = (unsigned char)buf[i];
                if (c >= 32 && c < 127)
                    printf("%c", c);   // Caràcter imprimible
                else
                    printf("\\x%02X", c); // Hex si no és imprimible
            }
            printf("\n");
            
            if(buf == "111") draw_image_file(0,0,Size_X,Size_Y,"../images/on.png");
            else draw_image_file(0,0,Size_X,Size_Y,"../images/off.png");

            fflush(stdout);
        } else if (n < 0 && errno != EAGAIN) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            running = 0;
        }
    }
    return NULL;
}


// --- Bucle de transmissió -------------------------------------
void tx_loop() {
    char input[256];

    printf("\nEscriu text i prem Enter per enviar al PIC.\n");
    printf("Comandes especials:\n");
    printf("  :quit  -> sortir\n");
    printf("  :ping  -> envia '?'\n\n");

//    int initscreen = write(fd,"", 1);
//    if (initscreen < 0) fprintf(stderr, "[ERROR INITIALIZING SCREEN\n", strerror);

    while (running) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            running = 0;
            break;
        }

        // Elimina el newline final
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, ":quit") == 0) {
            running = 0;
            break;
        }

        char payload[258];
        int  len;

        if (strcmp(input, ":ping") == 0) {
            strcpy(payload, "?");
            len = 1;
        } else {
            len = snprintf(payload, sizeof(payload), "%s\r\n", input);
        }

        int written = write(fd, payload, len);
        if (written < 0)
            fprintf(stderr, "[ERROR TX] %s\n", strerror(errno));
        else
            printf("[TX] %.*s\n", len, payload);
    }
}

/*
int main() {
    init(); // inicialitzar pantalla
    draw_image_file(0,0,Size_X,Size_Y,"../images/off.png");


    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

    // Inicia el thread de recepció
    pthread_t rx;
    if (pthread_create(&rx, NULL, rx_thread, NULL) != 0) {
        fprintf(stderr, "[ERROR] No s'ha pogut crear el thread RX\n");
        close(fd);
        return EXIT_FAILURE;
    }

    tx_loop();

    // Neteja
    running = 0;
    pthread_join(rx, NULL);
    close(fd);
    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}

*/

int main() {
    init(); // inicialitzar pantalla
    draw_image_file(0, 0, Size_X, Size_Y, "../images/off.png");

    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

    while (1) {
        // --- Envia missatge 'x' ---
        if (write(fd, "x", 1) != 1) {
            fprintf(stderr, "[ERROR TX] No s'ha pogut enviar 'x'\n");
            break;
        }

        // --- Espera bloquejant per rebre dades ---
        char buf[16];
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n < 0) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            break;
        }
        buf[n] = '\0'; // tancar string

        // --- Decideix quina imatge imprimir ---
        if (strcmp(buf, "111") == 0) {
            draw_image_file(0, 0, Size_X, Size_Y, "../images/on.png");
        } else {
            draw_image_file(0, 0, Size_X, Size_Y, "../images/off.png");
        }

        // --- Envia segon missatge 'x' després de mostrar la imatge ---
        if (write(fd, "x", 1) != 1) {
            fprintf(stderr, "[ERROR TX] No s'ha pogut enviar 'x'\n");
            break;
        }

        // --- Opcional: petit delay abans de tornar a començar ---
        usleep(100000); // 100 ms
    }

    close(fd);
    printf("[OK] Procés finalitzat.\n");
    return EXIT_SUCCESS;
}
