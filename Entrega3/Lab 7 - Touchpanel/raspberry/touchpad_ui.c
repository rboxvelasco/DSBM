#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>
#include "lib_tft.h"
#include "lib_uart.h"
#include "colors.h"

#define SERIAL_PORT "/dev/ttyAMA0"
#define BAUD_RATE   B9600
#define PIN_SEND_PERMISSION 13  // GPIO13 (BCM)
#define PACKET_SIZE 5
#define SYNC_BYTE   0xAA

// --- Variables globals ----------------------------------------
static int  fd          = -1;
static volatile int running = 1;
static volatile int state = 0; // 0 pagina d'incici; 1 pagina main

Font Font5x7_struct = { Font5x7, 5, 7 };

void config_Pin() {
    wiringPiSetupGpio();
    pinMode(PIN_SEND_PERMISSION, OUTPUT);
    // Inicialment a zero
    digitalWrite(PIN_SEND_PERMISSION, LOW);
}

void set_Pin(int value) {
    digitalWrite(PIN_SEND_PERMISSION, value ? HIGH : LOW);
}

void draw_close_button() {
    clear_screen(GREEN);
    draw_image(160, 0, 80, 40, "../../../assets/close.png");
    draw_text(65, 5, "Close", RED, Font5x7_struct, 3);
}

void draw_start_button() {
    clear_screen(GREEN);
    draw_circle(Size_X/2,Size_Y/2,80,BLACK);
    draw_circle_filled(Size_X/2,Size_Y/2,79,SKYBLUE);
    draw_text(Size_X/2-60, Size_Y/2-10, "Iniciar", BLACK, Font5x7_struct,3);
}

void init_UI () {
    init_TFT();
    draw_start_button();
}

void handle_touch(int sx, int sy) {
    if (sx < 0 || sy < 0) return;

    switch (state) {
        case 0: // pagina boto iniciar
            if ((sx-Size_X/2)*(sx-Size_X/2) + (sy-Size_Y/2)*(sy-Size_Y/2) < 80*80) {
                state = 1;
                set_Pin(0);
                draw_close_button();
                sleep(1);
                set_Pin(1);
            }
            break;
        case 1: // pagina main
            if (sx >= 150 && sy < 50) {
                state = 0;
                set_Pin(0);
                draw_start_button();
                sleep(1);
                set_Pin(1);
            }
            else {
                draw_pixel_scaled(sx,sy,RED,3);
            }
            break;
    }
}

int main() {
    config_Pin();
    init_UI();

    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

    // Posem el pin a 1 perque el PIC envii dades
    set_Pin(1);
uint8_t buf[PACKET_SIZE];
int filled = 0;

while (running) {
    uint8_t byte;
    int n = read(fd, &byte, 1);

    if (n < 0 && errno != EAGAIN) {
        fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
        running = 0;
        break;
    }
    if (n <= 0) continue;

    if (filled < PACKET_SIZE) {
        // Omplim el buffer fins tenir 5 bytes
        buf[filled++] = byte;
    }

    if (filled == PACKET_SIZE) {
        if (buf[PACKET_SIZE - 1] == SYNC_BYTE) {
            // Paquet vàlid: processar
            uint16_t x = (uint16_t)(buf[0] | (buf[1] << 8));
            uint16_t y = (uint16_t)(buf[2] | (buf[3] << 8));

            int sx, sy;
            touch_to_screen(x, y, &sx, &sy);
            printf("[RX] touch=(%u,%u) -> screen=(%d,%d)\n", x, y, sx, sy);
            handle_touch(sx, sy);
            fflush(stdout);

            filled = 0; // reset net per al proper paquet

        } else {
            // Paquet invàlid: shiftegem el buffer una posició
            // i esperem un byte nou per re-sincronitzar
            printf("[WARN] Desincronitzat, shiftejant buffer...\n");
            memmove(buf, buf + 1, PACKET_SIZE - 1);
            filled = PACKET_SIZE - 1;
        }
    }
}
    /*
    while(1) {
        char packet[4];
        int received = 0;

        while (running) {
            char byte;
            int n = read(fd, &byte, 1);
            if (n > 0) {
                packet[received++] = byte;
                if (received == 4) {
                    uint16_t x = (uint16_t)(packet[0] | (packet[1] << 8));
                    uint16_t y = (uint16_t)(packet[2] | (packet[3] << 8));

                    int sx, sy;
                    touch_to_screen(x, y, &sx, &sy);
                    printf("[RX] touch=(%u, %u)  ->  screen=(%d, %d)\n", x, y, sx, sy);
                    handle_touch(sx, sy);
                    fflush(stdout);
                    received = 0;
                }
            } else if (n < 0 && errno != EAGAIN) {
                fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
                running = 0;
            }
        }
    }
    */

    // Neteja
    running = 0;
    close(fd);
    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}
