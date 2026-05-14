#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "lib_tft.h"
#include "lib_uart.h"

// ── Configuració socket ────────────────────────────────────────────────────────
#define SOCKET_PATH     "/tmp/letter_recognition.sock"
#define PROTO_MAGIC     0xDEADBEEF
// Timeout de 10 s per si el servidor Python no respon
#define SOCKET_TIMEOUT_S  10

// --- Variables globals ----------------------------------------
int matrix[Size_X][Size_Y];

static volatile int game_state = 1; // 0 pagina d'incici; 1 pagina main
static volatile int light_state = 0; // 0 dark mode; 1 bright mode
uint8_t ligth_high_th = 250;
uint8_t light_low_th = 240;
static volatile int radi_button = 5;

Font DefaultFont = { Font5x7, 5, 7 };

typedef struct {
    int x0, y0, x1, y1;
    int color1, color2;
    int border_color1, border_color_2;
    char* text;
    char* path;
} Button;

Button CloseButton = {160, 2, 238, 40, RED, WHITE, 0, 0,"", "../../assets/close.png"};
Button ClearButton = {10,283,110,310, SKYBLUE, SKYBLUE, BLACK, WHITE, "Clear", "../../assets/fons.png"};
Button ProcessButton = {130,283,230,310, SKYBLUE, SKYBLUE, BLACK, WHITE, "Process", "../../assets/fons.png"};


//  SOCKET  –  envia la matriu al servidor Python i espera ACK
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Escriu exactament `n` bytes al descriptor `fd`.
 * Retorna 0 si tot OK, -1 en cas d'error.
 */
static int write_all(int fd, const void *buf, size_t n) {
    const uint8_t *p = buf;
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w <= 0) return -1;
        p += w;
        n -= (size_t)w;
    }
    return 0;
}

/**
 * Connecta al servidor Python, envia la matriu + light_state i espera ACK.
 *
 * Retorna:
 *   0  – OK (Python ha escrit el JSON)
 *  -1  – error de connexió / timeout
 *   1  – el servidor ha retornat error intern
 */
int send_matrix_to_python(void) {
    // ── Obre el socket ────────────────────────────────────────────────────────
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[SOCKET] socket()");
        return -1;
    }

    // Timeout de lectura per no bloquejar indefinidament
    struct timeval tv = { .tv_sec = SOCKET_TIMEOUT_S, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[SOCKET] connect() – el servidor Python arranca?");
        close(sock);
        return -1;
    }

    // ── Capçalera: magic(4) + light(1) + size_x(4) + size_y(4) = 13 bytes ───
    //  Usem little-endian explícit (igual que el Python amb struct "<IBII")
    uint32_t magic  = PROTO_MAGIC;
    uint8_t  light  = (uint8_t)light_state;
    uint32_t sx     = (uint32_t)Size_X;
    uint32_t sy     = (uint32_t)Size_Y;

    if (write_all(sock, &magic, 4) < 0 ||
        write_all(sock, &light, 1) < 0 ||
        write_all(sock, &sx,    4) < 0 ||
        write_all(sock, &sy,    4) < 0)
    {
        perror("[SOCKET] write header");
        close(sock);
        return -1;
    }

    // ── Matriu en ordre row-major (y extern, x intern) ────────────────────────
    //  Enviem 1 byte per cel·la (0 o 1).
    for (int y = 0; y < Size_Y; y++) {
        for (int x = 0; x < Size_X; x++) {
            uint8_t val = (uint8_t)(matrix[x][y] ? 1 : 0);
            if (write_all(sock, &val, 1) < 0) {
                perror("[SOCKET] write matrix");
                close(sock);
                return -1;
            }
        }
    }

    // ── Espera ACK (1 byte) ───────────────────────────────────────────────────
    uint8_t ack = 0xFF;
    ssize_t r = read(sock, &ack, 1);
    close(sock);

    if (r <= 0) {
        fprintf(stderr, "[SOCKET] Timeout o error esperant ACK\n");
        return -1;
    }

    return (ack == 0x00) ? 0 : 1;
}


/*******************       DRAW FUNCTIONS      ***********/
void draw_close_button() {
    draw_image(CloseButton.x0, CloseButton.y0, CloseButton.x1-CloseButton.x0, CloseButton.y1-CloseButton.y0, "../../assets/close.png");
    // draw_text(65, 5, "Close", light_state ? CloseButton.color1 : CloseButton.color2, DefaultFont, 3);
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
    //draw_rectangle(button.x0-1, button.y0-1, button.x1+1, button.y1+1, color2);
    // Draw rectangle
    // draw_rectangle_filled(button.x0, button.y0, button.x1, button.y1, button.color1);
    //int x0p = button.x0, y0p = button.y0, x1p = button.x1, y1p = button.y1;
    //printf("x0: %d, y0: %d, x1: %d, y1: %d, color1: %d, color2: %d",
//	            x0p, y0p, x1p, y1p, button.color1, button.color2);

    //printf("x0: %d, y0: %d, x1: %d, y1: %d, color1: %d, color2: %d",
//            x0p, y0p, x1p, y1p, button.color1, button.color2);
    //draw_rectangle_filled(x0p,y0p, x1p, y1p, SKYBLUE);
    //draw_rectangle_filled(10, 250, 80, 300, SKYBLUE);
    //SPI_TFT_region(Size_X/2, Size_Y/2, 20,  20, color);
    // Draw centered text
    // draw_text((button.x1-button.x0)/2-60, (button.x1-button.x0)/2-10, button.text, color2, DefaultFont, scale_text);
    // Draw backround image
    draw_image(button.x0, button.y0, button.x1-button.x0, button.y1-button.y0, button.path); 
    // Draw text
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
    print_backround();
    if (game_state) {
        draw_close_button();
        draw_button(ClearButton);
        draw_button(ProcessButton);
        redraw_matrix();
    }
    else draw_start_button();
    sleep(2);
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

// ── Feedback visual mentre es processa ────────────────────────────────────────
static void draw_processing_overlay(const char *msg) {
    int cx = Size_X / 2;
    int cy = Size_Y / 2;
    int bw = 180, bh = 40;
    draw_rectangle_filled(cx - bw/2, cy - bh/2, bw, bh,
                          light_state ? WHITE : BLACK);
    draw_rectangle(cx - bw/2 - 1, cy - bh/2 - 1, bw + 2, bh + 2,
                   light_state ? BLACK : WHITE);
    draw_text(cx - bw/2 + 10, cy - 8, msg,
              light_state ? BLACK : WHITE, DefaultFont, 2);
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
            else if (button_pressed(sx,sy,ClearButton)) {
                // Reiniciar la matriu i imprimir pantalla
		init_matrix();
                print_screen();
            }
            else if (button_pressed(sx, sy, ProcessButton)) {
                // ── PROCESSAR: enviar la matriu al servidor Python ────────────
		draw_processing_overlay("Processant...");

                int ret = send_matrix_to_python();

                if (ret == 0) {
                    printf("[OK] Reconeixement completat. JSON a /tmp/recognition_result.json\n");
                    draw_processing_overlay("Fet!");
                } else {
                    fprintf(stderr, "[ERR] send_matrix_to_python() -> %d\n", ret);
                    draw_processing_overlay("Error!");
                }
                fflush(stdout);
		
		init_matrix();
                // Petita pausa perquè l'usuari vegi el missatge, després repintem
                sleep(2);
		
                print_screen();
            }
            else {
                // ── Toc vàlid al canvas → dibuixar i emmagatzemar ────────────
                if (light_state) draw_pixel_scaled(sx, sy, RED,   3);
                else             draw_pixel_scaled(sx, sy, WHITE, 3);

                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = sx + dx;
                        int ny = sy + dy;
                        if (nx >= 0 && nx < Size_X && ny >= 0 && ny < Size_Y)
                            matrix[nx][ny] = 1;
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
        uint16_t light;
        int ret = read_message(&x, &y, &light);
        if (ret < 0) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            break;
        }
	set_Pin(0);
        // El convertim a coordenades de la TFT
        int sx, sy;
        touch_to_screen(x, y, &sx, &sy);
        printf("[RX] touch=(%u, %u) -> screen=(%d, %d); light=%u\n", x, y, sx, sy, light);
        fflush(stdout);

        // Logica i processat del missatge
        handle_touch(sx, sy);
        handle_light(light);
	set_Pin(1);
    }

    // Neteja
    if (close_UART() < 0) {
        fprintf(stderr, "[ERROR] Error tancant UART\n");
        return EXIT_FAILURE;
    } else printf("[OK] Port tancat. Fins aviat!\n");

    return EXIT_SUCCESS;
}
