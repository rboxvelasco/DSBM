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

    int estat = 0;

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
        if (buf[1] == '1' && estat == 0) {
            printf("ON\n");
            estat = 1;
            draw_image_file(0, 0, Size_X, Size_Y, "../images/on.png");
        } else if (estat == 1 && buf[1] == '0') {
            printf("OFF\n");
            estat = 0;
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
