#include "lib_uart.h"

static int  fd          = -1;

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
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        close(fd);
        return -1;
    }

    printf("[OK] Port obert: %s @ 9600 baud 8N1\n", port);
    return fd;
}

int start_UART() {
    fd = open_serial(SERIAL_PORT);
    if (fd < 0) return -1;
    else return 0;
}

int close_UART() {
    return close(fd);
}

int read_message(uint16_t *x, uint16_t *y, uint16_t *light) {
    // Buffer per emmagatzemar el missatge
    uint8_t buf[PACKET_SIZE];  
    int filled = 0;
    // Llegeix bytes fins que es completa un paquet vàlid o hi ha un error
    while (1) {
        uint8_t byte;
        int n = read(fd, &byte, 1);
        if (n > 0) {
            buf[filled++] = byte;
            if (filled == PACKET_SIZE) {
                // Canvi 3: validar els dos 0xAA i els bytes alts
                if (buf[0] == 0xAA       &&
                    buf[2] <= 0x03       &&
                    buf[4] <= 0x03       &&
                    buf[7] == 0xAA) {
                    // Canvi 4: índexs desplaçats +1 per saltar el 0xAA inicial
                    *x  = (uint16_t)(buf[1] | (buf[2] << 8));
                    *y  = (uint16_t)(buf[3] | (buf[4] << 8));
                    *light = (uint16_t) (buf[5] | (buf[6] << 8));
                    tcflush(fd, TCIFLUSH);
		    return 0; 

                } else {
                    // Re-sincronització: shift d'un byte
                    memmove(buf, buf + 1, PACKET_SIZE - 1);
                    filled = PACKET_SIZE - 1;
                }
            }
        } else if (n < 0 && errno != EAGAIN) {
            return -1;
        }
    }
}

void config_Pin() {
    wiringPiSetupGpio();
    pinMode(PIN_SEND_PERMISSION, OUTPUT);
    // Inicialment a zero
    digitalWrite(PIN_SEND_PERMISSION, LOW);
}

void set_Pin(int value) {
    digitalWrite(PIN_SEND_PERMISSION, value ? HIGH : LOW);
}
