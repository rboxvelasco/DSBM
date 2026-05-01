#include "lib_uart.h"

// Gestionar comunicació UART mitjançant crides POSIX
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

    // Habilita recepcio, ignora modem controls
    tty.c_cflag |= CREAD | CLOCAL;

    // Mode raw (sense processament especial)
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);        // Sense control de flux software
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR  | IGNCR | ICRNL);
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