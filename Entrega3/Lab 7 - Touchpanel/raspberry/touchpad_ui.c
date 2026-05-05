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

// --- Variables globals ----------------------------------------
static int  fd          = -1;
static volatile int running = 1;

Font Font5x7_struct = { Font5x7, 5, 7 };

void restart_UI() {
    clear_screen(GREEN);
    draw_image(160, 0, 80, 40, "../../../assets/close.png");
}

void init_UI () {
    init_TFT();
    restart_UI();
    draw_circle_filled(10,10,5,WHITE);
    draw_circle_filled(20,10,5,WHITE);
    draw_triangle_filled(10,10,20,10,15,30,SKYBLUE);
    draw_text(65, 5, "Close", RED, Font5x7_struct, 3);
}


int main() {
    init_UI();

    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

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
                    if (sx >= 150 && sy < 50) {
                        restart_UI();
			sleep(5);
                    }
                    else {
                        draw_pixel_scaled(sx,sy,RED,3);
                    }
                    fflush(stdout);
                    received = 0;
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

