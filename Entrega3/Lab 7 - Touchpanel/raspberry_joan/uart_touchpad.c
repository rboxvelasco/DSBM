#include <stdint.h>
#include <pthread.h>

#include "lib_tft.h"
#include "lib_uart.h"
#include "colors.h"

/****************** Variables globals *************************/
static int  fd          = -1;
static volatile int running = 1;

Font Font5x7_struct = { Font5x7, 5, 7 };

/**************** Rececpició i pintat de coordenades **********************/

void *rx_thread(void *arg) {
    (void)arg;
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
                draw_pixel_scaled(sx,sy,RED,3);
                fflush(stdout);
                received = 0;
            }
        } else if (n < 0 && errno != EAGAIN) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            running = 0;
        }
    }
    return NULL;
}


int main() {
    init_TFT();
    clear_screen(GREEN);

    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

    // Inicia el thread de recepcio
    pthread_t rx;
    if (pthread_create(&rx, NULL, rx_thread, NULL) != 0) {
        fprintf(stderr, "[ERROR] No s'ha pogut crear el thread RX\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // Neteja
    running = 0;
    pthread_join(rx, NULL);
    close(fd);
    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}
