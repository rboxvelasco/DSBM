#include <stdint.h>
#include <pthread.h>

#include "lib_tft.h"
#include "lib_uart.h"
#include "colors.h"

/****************** Variables globals *************************/
static int  fd          = -1;
static volatile int running = 1;


/**************** Calibració touchpad-pantalla **********************/

/*
Coeficients escalats x65536 per evitar floats (apta per MCU/embedded)
   screen_x = 0.382295 * touch_x + 0.011948 * touch_y - 64.2113
   screen_y = 0.014370 * touch_x + 0.569150 * touch_y - 92.7876
*/
#define CAL_A   25055    /*  0.382295 * 65536 */
#define CAL_B     783    /*  0.011948 * 65536 */
#define CAL_C  -4208630  /* -64.2113  * 65536 */

#define CAL_D     942    /*  0.014370 * 65536 */
#define CAL_E   37299    /*  0.569150 * 65536 */
#define CAL_F  -6079841  /* -92.7876  * 65536 */

// "Tradueix" les coordenades detectades pel touchpad a les de la TFT
static inline void touch_to_screen(uint16_t touch_x, uint16_t touch_y, int *screen_x, int *screen_y) {
    int32_t sx = (CAL_A * (int32_t)touch_x + CAL_B * (int32_t)touch_y + CAL_C) >> 16;
    int32_t sy = (CAL_D * (int32_t)touch_x + CAL_E * (int32_t)touch_y + CAL_F) >> 16;

    // Clamp a l'àrea de pantalla
    if (sx < 0)      sx = 0;
    if (sx > Size_X) sx = Size_X;
    if (sy < 0)      sy = 0;
    if (sy > Size_Y) sy = Size_Y;

    *screen_x = (int)sx;
    *screen_y = (int)sy;
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
                draw_pixel(sx-1, sy-1, RED);
                draw_pixel(sx, sy-1, RED);
                draw_pixel(sx+1, sy-1, RED);

                draw_pixel(sx-1, sy, RED);
                draw_pixel(sx, sy, RED);
                draw_pixel(sx+1, sy, RED);

                draw_pixel(sx-1, sy+1, RED);
                draw_pixel(sx, sy+1, RED);
                draw_pixel(sx+1, sy+1, RED);

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
    init();
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
