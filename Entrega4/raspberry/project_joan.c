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
#include "lib_uart_joan.h"


// --- Variables globals ----------------------------------------
static volatile int running = 1;
static volatile int game_state = 0; // 0 pagina d'incici; 1 pagina main
static volatile int light_state = 1; // 0 dark mode; 1 bright mode
uint8_t ligth_high_th = 250;
uint8_t light_low_th = 240;

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

/*******************       LOGIC FUNCTIONS      ***********/
void init_UI () {
    init_TFT();
    print_screen();
}

void handle_light(int8_t light) {
    if (light > ligth_high_th) {
        light_state = 0;
    } else if (light < light_low_th) {
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

/*******************       MAIN       ***********/
int main() {
    config_Pin();
    init_UI();
    if (start_UART() < 0) return EXIT_FAILURE;

    // Autoritzem el PIC que envii dades
    set_Pin(1);
    while (1) {
        // Llegim un missatge
        uint16_t x, y;
        int8_t light;
        int ret = read_message(&x, &y, &light);
        if (ret < 0) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            break;
        } 

        // El convertim a coordenades de la TFT
        int sx, sy;
        touch_to_screen(x, y, &sx, &sy);
        printf("[RX] touch=(%u, %u) -> screen=(%d, %d); light=%d\n", x, y, sx, sy, light);
        fflush(stdout);

        // Logica i processat del missatge
        handle_touch(sx, sy);
        handle_light(light);
    }

    // Neteja
    if (close_UART() < 0) {
        fprintf(stderr, "[ERROR] Error tancant UART\n");
        return EXIT_FAILURE;
    } else printf("[OK] Port tancat. Fins aviat!\n");
    
    return EXIT_SUCCESS;
}
