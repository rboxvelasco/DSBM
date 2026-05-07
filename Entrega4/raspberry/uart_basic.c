#include <stdlib.h>
#include <pthread.h>

#include "lib_uart.h"

/**************** Variables globals **********************/
static int  fd = -1;
static volatile int running = 1;


/********************* Codi UART **************************/

// Thread de recepcio: imprimeix per pantalla el que rebi per UART
void *rx_thread(void *arg) {
    (void)arg;
    char buf[64];

    while (running) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("[RX] ");
            for (int i = 0; i < n; i++) {
                unsigned char c = (unsigned char)buf[i];
                if (c >= 32 && c < 127) printf("%c", c); // Caracter imprimible
                else printf("\\x%02X", c); // Hex si no es imprimible
            }
            printf("\n");
            fflush(stdout);
        } else if (n < 0 && errno != EAGAIN) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            running = 0;
        }
    }
    return NULL;
}


// Bucle de transmissio: envia per UART el que s'escrigui per teclat
void tx_loop() {
    char input[256];

    printf("\nEscriu text i prem Enter per enviar al PIC.\n");
    printf("Comandes especials:\n");
    printf("  :quit  -> sortir\n");
    printf("  :ping  -> envia '?'\n\n");

    while (running) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            running = 0;
            break;
        }

        // Elimina el newline final
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, ":quit") == 0) {
            running = 0;
            break;
        }

        char payload[258];
        int  len;

        if (strcmp(input, ":ping") == 0) {
            strcpy(payload, "?");
            len = 1;
        } else {
            len = snprintf(payload, sizeof(payload), "%s\r\n", input);
        }

        int written = write(fd, payload, 1);
        if (written < 0) fprintf(stderr, "[ERROR TX] %s\n", strerror(errno));
        else printf("[TX] %.*s\n", len, payload);
    }
}


int main() {
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

    // Executar bucle de transmissió
    tx_loop();

    // Neteja
    running = 0;
    pthread_join(rx, NULL);
    close(fd);
    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}
