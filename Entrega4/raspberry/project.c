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
#define SOCKET_PATH      "/tmp/letter_recognition.sock"
#define PROTO_MAGIC      0xDEADBEEF
// Timeout de 10 s per si el servidor Python no respon
#define SOCKET_TIMEOUT_S  10

// --- Variables globals --------------------------------------------------------
int matrix[Size_X][Size_Y];

static volatile int game_state  = 0;  // 0: pàgina d'inici;  1: pàgina main
static volatile int light_state = 0;  // 0: dark mode;       1: bright mode

// [FIX] Tipus uint16_t per ser coherents amb el valor rebut per UART
// [FIX] Umbrals ajustats: el LDR arriba en 8 bits efectius (0–255) després del >>2 del PIC
uint16_t light_high_th = 200;
uint16_t light_low_th  = 150;

// [FIX] Radi reduït a 1 per evitar falsos positius als botons
static volatile int radi_button = 1;

// [FIX] Variables per a la interpolació de traços
static int prev_sx = -1;
static int prev_sy = -1;

// [FIX] Variables per al debounce de botons (evita activació amb un sol toc)
static int button_touch_count = 0;
static int last_button         = -1;  // 0=close, 1=clear, 2=process

Font DefaultFont = { Font5x7, 5, 7 };

typedef struct {
    int x0, y0, x1, y1;
    int color1, color2;
    int border_color1, border_color_2;
    char* text;
    char* path;
} Button;

Button CloseButton   = {160,  2, 238,  40, RED,     WHITE,   0,     0,     "",        "../../assets/close.png"};
Button ClearButton   = { 10, 283, 110, 310, SKYBLUE, SKYBLUE, BLACK, WHITE, "Clear",   "../../assets/fons.png"};
Button ProcessButton = {130, 283, 230, 310, SKYBLUE, SKYBLUE, BLACK, WHITE, "Process", "../../assets/fons.png"};


// ═══════════════════════════════════════════════════════════════════════════════
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

    // Capçalera: magic(4) + light(1) + size_x(4) + size_y(4) = 13 bytes
    uint32_t magic = PROTO_MAGIC;
    uint8_t  light = (uint8_t)light_state;
    uint32_t sx    = (uint32_t)Size_X;
    uint32_t sy    = (uint32_t)Size_Y;

    if (write_all(sock, &magic, 4) < 0 ||
        write_all(sock, &light, 1) < 0 ||
        write_all(sock, &sx,    4) < 0 ||
        write_all(sock, &sy,    4) < 0)
    {
        perror("[SOCKET] write header");
        close(sock);
        return -1;
    }

    // Matriu en ordre row-major (y extern, x intern): 1 byte per cel·la (0 o 1)
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

    // Espera ACK (1 byte)
    uint8_t ack = 0xFF;
    ssize_t r = read(sock, &ack, 1);
    close(sock);

    if (r <= 0) {
        fprintf(stderr, "[SOCKET] Timeout o error esperant ACK\n");
        return -1;
    }

    return (ack == 0x00) ? 0 : 1;
}


// ═══════════════════════════════════════════════════════════════════════════════
//  DRAW FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

void draw_close_button(void) {
    draw_image(CloseButton.x0, CloseButton.y0,
               CloseButton.x1 - CloseButton.x0,
               CloseButton.y1 - CloseButton.y0,
               "../../assets/close.png");
}

void draw_start_button(void) {
    int color_text = light_state ? BLACK : WHITE;
    draw_circle(Size_X / 2, Size_Y / 2, 80, color_text);
    draw_circle_filled(Size_X / 2, Size_Y / 2, 79, SKYBLUE);
    draw_text(Size_X / 2 - 60, Size_Y / 2 - 10, "Iniciar", color_text, DefaultFont, 3);
}

void draw_button(Button button) {
    int color2 = light_state ? button.border_color1 : button.border_color_2;

    // Imatge de fons
    draw_image(button.x0, button.y0,
               button.x1 - button.x0,
               button.y1 - button.y0,
               button.path);

    // Text centrat
    const char *label  = button.text;
    int scale_text = 2;
    int text_w = strlen(label) * (DefaultFont.width * scale_text + scale_text);
    int text_h = DefaultFont.height * scale_text;
    int tx = button.x0 + (button.x1 - button.x0 - text_w) / 2;
    int ty = button.y0 + (button.y1 - button.y0 - text_h) / 2;
    draw_text(tx, ty, label, color2, DefaultFont, scale_text);
}

void print_background(void) {
    light_state ? clear_screen(GREEN) : clear_screen(BLACK);
}

void redraw_matrix(void) {
    int color = light_state ? RED : WHITE;
    for (int x = 0; x < Size_X; x++) {
        for (int y = 0; y < Size_Y; y++) {
            if (matrix[x][y]) {
                draw_pixel(x, y, color);
            }
        }
    }
}

// [FIX] Eliminat el sleep(2) genèric: ara print_screen() és immediata.
//       El sleep es posa NOMÉS on té sentit (feedback de "Fet!" / "Error!").
void print_screen(void) {
    print_background();
    if (game_state) {
        draw_close_button();
        draw_button(ClearButton);
        draw_button(ProcessButton);
        redraw_matrix();
    } else {
        draw_start_button();
    }
}

// Feedback visual mentre es processa
static void draw_processing_overlay(const char *msg) {
    int cx = Size_X / 2;
    int cy = Size_Y / 2;
    int bw = 180, bh = 40;
    draw_rectangle_filled(cx - bw / 2, cy - bh / 2, bw, bh,
                          light_state ? WHITE : BLACK);
    draw_rectangle(cx - bw / 2 - 1, cy - bh / 2 - 1, bw + 2, bh + 2,
                   light_state ? BLACK : WHITE);
    draw_text(cx - bw / 2 + 10, cy - 8, msg,
              light_state ? BLACK : WHITE, DefaultFont, 2);
}


// ═══════════════════════════════════════════════════════════════════════════════
//  LOGIC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

int button_pressed(int x, int y, Button button) {
    return x >= button.x0 - radi_button && x <= button.x1 + radi_button &&
           y >= button.y0 - radi_button && y <= button.y1 + radi_button;
}

// [FIX] Tipus corregit a uint16_t (era int8_t, que truncava el valor)
void handle_light(uint16_t light) {
    int prev = light_state;
    if (light > light_high_th) {
        light_state = 0;  // dark mode
    } else if (light < light_low_th) {
        light_state = 1;  // bright mode
    }
    if (light_state != prev) {
        print_screen();
    }
}

void init_matrix(void) {
    memset(matrix, 0, sizeof(matrix));
}

// [FIX] Reseteja l'estat de debounce i el punt anterior de traç
static void reset_interaction_state(void) {
    prev_sx            = -1;
    prev_sy            = -1;
    button_touch_count = 0;
    last_button        = -1;
}

// [FIX] Marca una línia de píxels a la matriu (Bresenham) per coherència
//       entre el que es pinta a la TFT i el que s'envia al servidor Python.
static void matrix_draw_line(int x0, int y0, int x1, int y1) {
    int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        // Marca el punt i el seu entorn 3×3 a la matriu
        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int nx = x0 + kx, ny = y0 + ky;
                if (nx >= 0 && nx < Size_X && ny >= 0 && ny < Size_Y)
                    matrix[nx][ny] = 1;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void handle_touch(int sx, int sy) {
    // Coordenades invàlides (no-touch): reseteja el punt anterior però
    // NO reseteja el debounce, per no perdre el recompte entre mostres.
    if (sx < 0 || sy < 0) {
        prev_sx = -1;
        prev_sy = -1;
        return;
    }

    switch (game_state) {

        // ── Pàgina d'inici ──────────────────────────────────────────────────
        case 0:
            if ((sx - Size_X / 2) * (sx - Size_X / 2) +
                (sy - Size_Y / 2) * (sy - Size_Y / 2) < 80 * 80)
            {
                game_state = 1;
                reset_interaction_state();
                print_screen();
            }
            break;

        // ── Pàgina main ─────────────────────────────────────────────────────
        case 1:

            // ── Botó Close ──────────────────────────────────────────────────
            if (button_pressed(sx, sy, CloseButton)) {
                // [FIX] Debounce: cal 2 tocs consecutius al mateix botó
                if (last_button == 0) {
                    button_touch_count++;
                } else {
                    last_button        = 0;
                    button_touch_count = 1;
                }
                if (button_touch_count >= 2) {
                    game_state = 0;
                    reset_interaction_state();
                    print_screen();
                }
            }

            // ── Botó Clear ──────────────────────────────────────────────────
            else if (button_pressed(sx, sy, ClearButton)) {
                if (last_button == 1) {
                    button_touch_count++;
                } else {
                    last_button        = 1;
                    button_touch_count = 1;
                }
                if (button_touch_count >= 2) {
                    init_matrix();
                    reset_interaction_state();
                    print_screen();
                }
            }

            // ── Botó Process ────────────────────────────────────────────────
            else if (button_pressed(sx, sy, ProcessButton)) {
                if (last_button == 2) {
                    button_touch_count++;
                } else {
                    last_button        = 2;
                    button_touch_count = 1;
                }
                if (button_touch_count >= 2) {
                    draw_processing_overlay("Processant...");

                    int ret = send_matrix_to_python();

                    if (ret == 0) {
                        printf("[OK] Reconeixement completat.\n");
                        draw_processing_overlay("Fet!");
                    } else {
                        fprintf(stderr, "[ERR] send_matrix_to_python() -> %d\n", ret);
                        draw_processing_overlay("Error!");
                    }
                    fflush(stdout);

                    // [FIX] sleep(2) NOMÉS aquí, per mostrar el feedback
                    sleep(2);

                    init_matrix();
                    reset_interaction_state();
                    print_screen();
                }
            }

            // ── Canvas: dibuixar traç ────────────────────────────────────────
            else {
                // [FIX] Qualsevol toc al canvas reseteja el debounce dels botons
                button_touch_count = 0;
                last_button        = -1;

                int color = light_state ? RED : WHITE;

                if (prev_sx >= 0 && prev_sy >= 0) {
                    // [FIX] Interpolació: dibuixem una línia gruixuda entre el
                    //       punt anterior i l'actual, tant a la TFT com a la matriu.
                    //       El bucle amb offset ±1 simula un pinzell de 3px.
                    for (int t = -1; t <= 1; t++) {
                        draw_line(prev_sx + t, prev_sy, sx + t, sy, color);
                        draw_line(prev_sx, prev_sy + t, sx, sy + t, color);
                    }
                    matrix_draw_line(prev_sx, prev_sy, sx, sy);
                } else {
                    // Primer punt del traç: nomes pintem el punt inicial
                    draw_pixel_scaled(sx, sy, color, 3);
                    for (int ky = -1; ky <= 1; ky++) {
                        for (int kx = -1; kx <= 1; kx++) {
                            int nx = sx + kx, ny = sy + ky;
                            if (nx >= 0 && nx < Size_X && ny >= 0 && ny < Size_Y)
                                matrix[nx][ny] = 1;
                        }
                    }
                }

                prev_sx = sx;
                prev_sy = sy;
            }
            break;
    }
}

void init_UI(void) {
    init_TFT();
    init_matrix();
    reset_interaction_state();
    print_screen();
}


// ═══════════════════════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main(void) {
    config_Pin();
    init_UI();

    if (start_UART() < 0) return EXIT_FAILURE;

    // Autoritzem el PIC que comenci a enviar dades
    set_Pin(1);

draw_pixel_scaled(20, 20, BLUE, 5);
draw_pixel_scaled(20, 300, RED, 5);
draw_pixel_scaled(220, 20, GREEN, 5);
draw_pixel_scaled(220, 300, PINK, 5);

    while (1) {
        uint16_t x, y, light;

        int ret = read_message(&x, &y, &light);
        if (ret < 0) {
            fprintf(stderr, "[ERROR RX] %s\n", strerror(errno));
            break;
        }

        set_Pin(0);

        // Convertim a coordenades de la TFT
        int sx, sy;
        touch_to_screen(x, y, &sx, &sy);
//        printf("[RX] touch=(%u,%u) -> screen=(%d,%d)  light=%u\n", x, y, sx, sy, light);
        printf("[RX] touch=(%u,%u)\n", x, y);
        fflush(stdout);

        // Lògica principal
        handle_touch(sx, sy);

        // [FIX] handle_light rep uint16_t, igual que la variable 'light'
        handle_light(light);

        set_Pin(1);
    }

    if (close_UART() < 0) {
        fprintf(stderr, "[ERROR] Error tancant UART\n");
        return EXIT_FAILURE;
    }

    printf("[OK] Port tancat. Fins aviat!\n");
    return EXIT_SUCCESS;
}
