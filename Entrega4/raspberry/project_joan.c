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
int matrix[Size_X][Size_Y];

static volatile int game_state = 0; // 0 pagina d'incici; 1 pagina main
static volatile int light_state = 1; // 0 dark mode; 1 bright mode
uint8_t ligth_high_th = 250;
uint8_t light_low_th = 240;
static volatile int radi_button = 5;

Font DefaultFont = { Font5x7, 5, 7 };

typedef struct {
    int x0, y0, x1, y1;
    int color1, color2;
    int border_color1, border_color_2;
    char* text;
} Button;

Button CloseButton = {160, 2, 238, 40, RED, WHITE, 0, 0,""};
Button ResetButton = {10,283,110,318, SKYBLUE, SKYBLUE, BLACK, WHITE, "Clear"};
Button ProcessButton = {130,283,230,318, SKYBLUE, SKYBLUE, BLACK, WHITE, "Process"};

/*******************       DRAW FUNCTIONS      ***********/
void draw_close_button() {
    draw_image(CloseButton.x0, CloseButton.y0, CloseButton.x1-CloseButton.x0, CloseButton.y1-CloseButton.y0, "../../assets/close.png");
    draw_text(65, 5, "Close", light_state ? CloseButton.color1 : CloseButton.color2, DefaultFont, 3);
}

void draw_start_button() {
    int color_text = light_state ? BLACK : WHITE;
    draw_circle(Size_X/2,Size_Y/2,80,color_text);
    draw_circle_filled(Size_X/2,Size_Y/2,79,SKYBLUE);
    draw_text(Size_X/2-60, Size_Y/2-10, "Iniciar", color_text, DefaultFont,3);
}

void draw_button(Button button) {
    int color = light_state? button.color1 : button.color2;
    int color2 = light_state ? button.border_color1 : button.border_color_2;
    // Draw border
    draw_rectangle(button.x0-1, button.y0-1, button.x1-button.x0+1, button.y1-button.y0+1, color2);
    // Draw rectangle
    draw_rectangle_filled(button.x0, button.y0, button.x1-button.x0, button.y1-button.y0, color);
    // Draw centered text
    // draw_text((button.x1-button.x0)/2-60, (button.x1-button.x0)/2-10, button.text, color2, DefaultFont, scale_text);
    const char* label = button.text;
    int scale_text = 2;
    int text_w = strlen(label) * (DefaultFont.width * scale_text + scale_text);
    int text_h = DefaultFont.height * scale_text;
    int tx = button.x0 + (button.x1 - button.x0 - text_w) / 2;
    int ty = button.y0 + (button.y1 - button.y0 - text_h) / 2;
    draw_text(tx, ty, label, color2, DefaultFont, scale_text);
}

void print_backround() {
    light_state ? clear_screen(GREEN) : clear_screen(BLACK);
}

void redraw_matrix() {
    int color = light_state ? RED : WHITE;
    for (int x = 0; x < Size_X; x++) {
        for (int y = 0; y < Size_Y; y++) {
            if (matrix[x][y]) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void print_screen() {
    // TODO: Afegir logica de invertir colors
    set_Pin(0);
    print_backround();
    if (game_state) {
        draw_close_button();
        draw_button(ResetButton);
        draw_button(ProcessButton);
        redraw_matrix();
    }
    else draw_start_button();
    sleep(1);
    set_Pin(1);
}

/*******************       LOGIC FUNCTIONS      ***********/
int button_pressed(int x, int y, Button button) {
    return x >= button.x0-radi_button && x <= button.x1+radi_button && y >= button.y0-radi_button && y <= button.y1+radi_button;
}

void handle_light(int8_t light) {
    int prev = light_state;
    if (light > ligth_high_th) {
        light_state = 0;
    } else if (light < light_low_th) {
        light_state = 1;
    }
    // Si canvia l'estat, escrivim el nou estat
    if (light_state != prev) {
        // Escriure a un fitxer l'estat
        // Repintar la pantalla
        print_screen();
    }
}

void init_matrix() {
    memset(matrix, 0, sizeof(matrix));
}

void handle_touch(int sx, int sy) {
    if (sx < 0 || sy < 0) return;
    switch (game_state) {
        case 0: // pagina boto iniciar
            if ((sx-Size_X/2)*(sx-Size_X/2) + (sy-Size_Y/2)*(sy-Size_Y/2) < 80*80) {
                game_state = 1;
                print_screen();
            }
            break;
        case 1: // pagina main
            if (button_pressed(sx,sy,CloseButton)) {
                game_state = 0;
                print_screen();
            }
            else if (button_pressed(sx,sy,ResetButton)) {
                // Reiniciar la matriu i imprimir pantalla
                init_matrix();
                print_screen();
            } 
            else if (button_pressed(sx,sy,ProcessButton)) {
                // Processar la matriu i imprimir pantalla
                print_screen();
            }
            else {
                // Valid touch, pintar i emmagatzemar
                if (light_state) draw_pixel_scaled(sx,sy,RED,3);
                else draw_pixel_scaled(sx,sy,WHITE,3);
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx >= 0 && dx < Size_X && dy >= 0 && dy < Size_Y)
                            matrix[sx+dx][sy+dy] = 1; 
                    }
                }
            }
            break;
    }
}

void init_UI () {
    init_TFT();
    init_matrix();
    print_screen();
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
