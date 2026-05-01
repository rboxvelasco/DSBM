#include <stdlib.h>

#include "lib_tft.h"
#include "lib_uart.h"
#include "colors.h"

Font Font5x7_struct = { Font5x7, 5, 7 };

static int  fd = -1;
static volatile int running = 1;


int main() {
    init(); // inicialitzar pantalla
    draw_image_file(0, 0, Size_X, Size_Y, "../../assets/off.png");

    fd = open_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Comprova:\n");
        fprintf(stderr, "  1. sudo raspi-config -> Interface Options -> Serial\n");
        fprintf(stderr, "  2. sudo usermod -aG dialout $USER  (i re-login)\n");
        return EXIT_FAILURE;
    }

    int estat = 0;

    while (1) {
        // Envia missatge 'x'
        if (write(fd, "x", 1) != 1) {
            fprintf(stderr, "[ERROR TX] No s'ha pogut enviar 'x'\n");
            break;
        }

        // Espera bloquejant per rebre dades
        char buf[16];
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n < 0) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            break;
        }
        buf[n] = '\0'; // tancar string

        // Decideix quina imatge imprimir
        if (estat == 0 && buf[1] == '1') {
            printf("ON\n");
            estat = 1;
            draw_image_file(0, 0, Size_X, Size_Y, "../../assets/on.png");
        } else if (estat == 1 && buf[1] == '0') {
            printf("OFF\n");
            estat = 0;
            draw_image_file(0, 0, Size_X, Size_Y, "../../assets/off.png");
        }

        // Envia segon missatge 'x' després de mostrar la imatge
        if (write(fd, "x", 1) != 1) {
            fprintf(stderr, "[ERROR TX] No s'ha pogut enviar 'x'\n");
            break;
        }

        // Petit delay abans de tornar a començar
        usleep(100000); // 100 ms
    }

    close(fd);
    printf("[OK] Procés finalitzat.\n");
    return EXIT_SUCCESS;
}
