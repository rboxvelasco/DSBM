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
#define PACKET_SIZE 7
#define SYNC_BYTE   0xAA
uint8_t LIGHT_HIGH = 250;
uint8_t LIGHT_LOW = 240;

// --- Variables globals ----------------------------------------
static int  fd          = -1;
static volatile int running = 1;
static volatile int game_state = 0; // 0 pagina d'incici; 1 pagina main
static volatile int light_state = 1; // 0 dark mode; 1 bright mode

Font Font5x7_struct = { Font5x7, 5, 7 };


/*******************       DRAW FUNCTIONS      ***********/
void draw_close_button() {
    int color_text = RED;
    if (!light_state) color_text = WHITE;
    draw_image(160, 0, 80, 40, "../../assets/close.png");
    draw_text(65, 5, "Close", color_text, Font5x7_struct, 3);
}

void draw_start_button() {
    int color_text = BLACK;
    if (!light_state) color_text = WHITE;

    draw_circle(Size_X/2,Size_Y/2,80,color_text);
    draw_circle_filled(Size_X/2,Size_Y/2,79,SKYBLUE);
    draw_text(Size_X/2-60, Size_Y/2-10, "Iniciar", color_text, Font5x7_struct,3);
}

void print_backround() {
    if (light_state == 0) clear_screen(BLACK);
    else clear_screen(GREEN);
}

void print_screen() {
    // TODO: Afegir logica de invertir colors
    print_backround();
    if (game_state) draw_close_button();
    else draw_start_button();
}

void init_UI () {
    init_TFT();
    print_screen();
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

void handle_light(int8_t light) {
    if (light > LIGHT_HIGH) {
        light_state = 0;
    } else if (light < LIGHT_LOW) {
        light_state = 1;
    }
}

void handle_touch(int sx, int sy) {
    if (sx < 0 || sy < 0) return;

    switch (game_state) {
        case 0: // pagina boto iniciar
            if ((sx-Size_X/2)*(sx-Size_X/2) + (sy-Size_Y/2)*(sy-Size_Y/2) < 80*80) {
                game_state = 1;
                set_Pin(0);
                print_screen();
                sleep(1);
                set_Pin(1);
            }
            break;
        case 1: // pagina main
            if (sx >= 150 && sy < 50) {
                game_state = 0;
                set_Pin(0);
                print_screen();
                sleep(1);
                set_Pin(1);
            }
            else {
                if (light_state) draw_pixel_scaled(sx,sy,RED,3);
                else draw_pixel_scaled(sx,sy,WHITE,3);
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

    while (running) {
        uint8_t buf[PACKET_SIZE];  // Canvi 2: uint8_t en lloc de char (per comparar 0xAA)
        int filled = 0;

        while (running) {
            uint8_t byte;
            int n = read(fd, &byte, 1);
            if (n > 0) {
                buf[filled++] = byte;

                if (filled == PACKET_SIZE) {
                    // Canvi 3: validar els dos 0xAA i els bytes alts
                    if (buf[0] == 0xAA       &&
                        buf[2] <= 0x03       &&
                        buf[4] <= 0x03       &&
                        buf[6] == 0xAA) {

                        // Canvi 4: índexs desplaçats +1 per saltar el 0xAA inicial
                        uint16_t x  = (uint16_t)(buf[1] | (buf[2] << 8));
                        uint16_t y  = (uint16_t)(buf[3] | (buf[4] << 8));
                        uint8_t light = (uint8_t) buf[5];

                        int sx, sy;
                        touch_to_screen(x, y, &sx, &sy);
                        printf("[RX] touch=(%u, %u) -> screen=(%d, %d); light=%d\n",
                            x, y, sx, sy, light);
                        handle_touch(sx, sy);
                        handle_light(light);
                        fflush(stdout);
                        filled = 0;

                    } else {
                        // Re-sincronització: shift d'un byte
                        memmove(buf, buf + 1, PACKET_SIZE - 1);
                        filled = PACKET_SIZE - 1;
                    }
                }
            } else if (n < 0 && errno != EAGAIN) {
                fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
                running = 0;
            }
        }
    }

    // Neteja
    running = 0;
    close(fd);
    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}
