#include <stdint.h>
#include <pthread.h>

#include "lib_tft.h"
#include "lib_uart.h"
#include "colors.h"

/****************** Variables globals *************************/
static int  fd          = -1;
static volatile int running = 1;

Font Font5x7_struct = { Font5x7, 5, 7 };

void restart_UI() {
    clear_screen(GREEN);
    draw_image(Size_X-20, Size_Y-10, Size_X, Size_Y, "../../assets/close.png");
}

void init_UI () {
    init_TFT();
    restart_UI();
    draw_circle_filled(10,10,5,WHITE);
    draw_circle_filled(20,10,5,WHITE);
    draw_triangle_filled(10,10,20,10,15,30,SKYBLUE);
    draw_text(Size_X-20,Size_Y-20, "Tancar", RED, Font5x7_struct, 2);
}

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
                if (sx >= Size_X-20 && sy > Size_Y-10) {
                    restart_UI();
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
    return NULL;
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
