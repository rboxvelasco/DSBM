// TFT Graphics Library
// Basada en el driver SPI d'Enric X. Martin Rull (FIB, DSBM, Març 2016)
// Llibreria de funcions grafiques d'alt nivell per a pantalla TFT 240x320
#include "lib_tft.h"

Font Font5x7_default = { Font5x7, 5, 7 };


// Prototips interns del driver (definits a driver.c)
void Config_Pins();
void SPI_TFT_Reset();
void SPI_TFT_pixel(int x, int y, int color);


/*************** INICIALITZACIO ************************************************/

// Inicialitza els pins GPIO i la pantalla TFT. Crida aquesta funcio una sola vegada a l'inici del programa.
void init_TFT() {
    Config_Pins();
    SPI_TFT_Reset();
    srand(time(NULL));
}

// Allibera els recursos GPIO. Crida aquesta funcio en tancar el programa.
void quit() {
    // wiringPi no requereix cleanup explicit,
    // pero es pot afegir logica de tancament aqui si cal.
}


/************* FUNCIONS BÀSIQUES *************************************************/

// Dibuixa un pixel
void draw_pixel(int x, int y, int color) {
    SPI_TFT_pixel(x, y, color);
}

// Dibuixa un pixel escalat (quadrat)
void draw_pixel_scaled(int x, int y, int color, int scale) {
    if (scale <= 1) {
        SPI_TFT_pixel(x, y, color);
        return;
    }
    int half = scale / 2;
    for (int dy = -half; dy <= half; dy++) {
        for (int dx = -half; dx <= half; dx++) {
            SPI_TFT_pixel(x + dx, y + dy, color);
        }
    }
}

// Dibuixa una línea horitzontal
void draw_hline(int x, int y, int length, int color) {
    SPI_TFT_region(x, y, x + length - 1, y, color);
}

// Dibuixa una línea vertical
void draw_vline(int x, int y, int length, int color) {
    SPI_TFT_region(x, y, x, y + length - 1, color);
}

// Dibuixa una línia arbitraria (algorisme de Bresenham)
void draw_line(int x0, int y0, int x1, int y1, int color) {
    int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


/************* FORMES GEOMETRIQUES *************************************************/

// Dibuixa la silueta d'un rectangle
void draw_rectangle(int x, int y, int width, int height, int color) {
    draw_hline(x,             y,              width,  color);  // costat superior
    draw_hline(x,             y + height - 1, width,  color);  // costat inferior
    draw_vline(x,             y,              height, color);  // costat esquerre
    draw_vline(x + width - 1, y,              height, color);  // costat dret
}

// Dibuixa un rectangle "ple"
void draw_rectangle_filled(int x, int y, int width, int height, int color) {
    SPI_TFT_region(x, y, x + width - 1, y + height - 1, color);
}

// Helpers interns que dibuixen els 8 octants simètrics
static void _circle_pixels(int cx, int cy, int x, int y, int color) {
    draw_pixel(cx + x, cy + y, color);
    draw_pixel(cx - x, cy + y, color);
    draw_pixel(cx + x, cy - y, color);
    draw_pixel(cx - x, cy - y, color);
    draw_pixel(cx + y, cy + x, color);
    draw_pixel(cx - y, cy + x, color);
    draw_pixel(cx + y, cy - x, color);
    draw_pixel(cx - y, cy - x, color);
}

static void _circle_hlines(int cx, int cy, int x, int y, int color) {
    draw_hline(cx - x, cy + y, 2 * x + 1, color);
    draw_hline(cx - x, cy - y, 2 * x + 1, color);
    draw_hline(cx - y, cy + x, 2 * y + 1, color);
    draw_hline(cx - y, cy - x, 2 * y + 1, color);
}

// Dibuixa la silueta d'un cercle (algorisme del punt mig)
void draw_circle(int cx, int cy, int r, int color) {
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        _circle_pixels(cx, cy, x, y, color);
        if (d < 0) { d += 2 * x + 3; }
        else       { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

// Dibuixa un cercle ple
void draw_circle_filled(int cx, int cy, int r, int color) {
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        _circle_hlines(cx, cy, x, y, color);
        if (d < 0) { d += 2 * x + 3; }
        else       { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

// ---- Triangle --------------------------------------------------------------

// Dibuixa la silueta d'un triangle
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

// Helper intern: triangle de base plana inferior (y1 == y2)
static void _fill_flat_bottom(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    float slope1 = (float)(x1 - x0) / (y1 - y0);
    float slope2 = (float)(x2 - x0) / (y2 - y0);
    float xa = (float)x0, xb = (float)x0;
    for (int y = y0; y <= y1; y++) {
        int xl = (int)xa, xr = (int)xb;
        if (xl > xr) { int t = xl; xl = xr; xr = t; }
        draw_hline(xl, y, xr - xl + 1, color);
        xa += slope1;
        xb += slope2;
    }
}

// Helper intern: triangle de base plana superior (y0 == y1)
static void _fill_flat_top(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    float slope1 = (float)(x2 - x0) / (y2 - y0);
    float slope2 = (float)(x2 - x1) / (y2 - y1);
    float xa = (float)x2, xb = (float)x2;
    for (int y = y2; y >= y0; y--) {
        int xl = (int)xa, xr = (int)xb;
        if (xl > xr) { int t = xl; xl = xr; xr = t; }
        draw_hline(xl, y, xr - xl + 1, color);
        xa -= slope1;
        xb -= slope2;
    }
}

// Dibuixa un triangle ple (algorisme de scanline amb divisió en dos subtriangles)
void draw_triangle_filled(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    // Ordenem els vèrtexs per y creixent (y0 <= y1 <= y2)
    if (y0 > y1) { int t; t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    if (y0 > y2) { int t; t = x0; x0 = x2; x2 = t; t = y0; y0 = y2; y2 = t; }
    if (y1 > y2) { int t; t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }

    if (y1 == y2) {
        // Ja té la base plana inferior
        _fill_flat_bottom(x0, y0, x1, y1, x2, y2, color);
    } else if (y0 == y1) {
        // Ja té la base plana superior
        _fill_flat_top(x0, y0, x1, y1, x2, y2, color);
    } else {
        // Cas general: dividim en dos subtriangles pel vèrtex intermedi
        int x3 = x0 + (int)((float)(y1 - y0) / (y2 - y0) * (x2 - x0));
        int y3 = y1;
        _fill_flat_bottom(x0, y0, x1, y1, x3, y3, color);
        _fill_flat_top(x1, y1, x3, y3, x2, y2, color);
    }
}


/***************** TEXT *****************************************************/

// Escriu un caràcter escalat a les coordenades (x, y)
void draw_char(int x, int y, unsigned char c, int color, Font font, int scale) {
    if (c < 32 || c > 127) return;
    if (scale < 1) scale = 1;

    int index = (c - 32) * font.width;
    for (int col = 0; col < font.width; col++) {
        unsigned char line = font.data[index + col];

        for (int row = 0; row < font.height; row++) {
            if (line & (1 << row)) {
                // dibuixa bloc escalat
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        draw_pixel(x + col * scale + dx, y + row * scale + dy, color);
                    }
                }
            }
        }
    }
}

// Escriu text escalat a les coordenades (x, y)
void draw_text(int x, int y, const char *text, int color, Font font, int scale) {
    if (scale < 1) scale = 1;

    int cursor_x = x;
    int cursor_y = y;
    int char_w = (font.width * scale) + scale;
    int char_h = (font.height * scale) + scale;

    while (*text) {
        // Salt de línia explícit '\n'
        if (*text == '\n') {
            cursor_x = x;
            cursor_y += char_h;
            text++;
            continue;
        }

        // Si el caràcter no cap, salt de línia automàtic
        if (cursor_x + char_w > Size_X) {
            cursor_x = x;
            cursor_y += char_h;
        }

        // Si sortim per baix de la pantalla, parem
        if (cursor_y + font.height * scale > Size_Y) break;

        draw_char(cursor_x, cursor_y, *text, color, font, scale);
        cursor_x += char_w;
        text++;
    }
}


/*************** IMATGES ************************************************/

// Pinta una imatge a les coordeades (x, y) amb amplada i alçada (dest_w, dest_h)
void draw_image (int x, int y, int dest_w, int dest_h, const char *path) {
    int src_w, src_h, channels;

    // Carrega la imatge i força 3 canals (RGB)
    unsigned char *data = stbi_load(path, &src_w, &src_h, &channels, 3);
    if (!data) {
        printf("Error: no s'ha pogut carregar '%s': %s\n", path, stbi_failure_reason());
        return;
    }

    // Resize nearest-neighbor + conversió RGB → RGB565 + draw
    for (int row = 0; row < dest_h; row++) {
        for (int col = 0; col < dest_w; col++) {
            int src_x = col * src_w / dest_w;
            int src_y = row * src_h / dest_h;
            int idx   = (src_y * src_w + src_x) * 3;

            int color = rgb_to_565(data[idx], data[idx+1], data[idx+2]);
            draw_pixel(x + col, y + row, color);
        }
    }

    stbi_image_free(data);
}


/*************** UTILITATS ************************************************/

// Converteix un color RGB (0-255 per canal) a format RGB565.
int rgb_to_565(int r, int g, int b) {
    int r5 = (r >> 3) & 0x1F;
    int g6 = (g >> 2) & 0x3F;
    int b5 = (b >> 3) & 0x1F;
    return (r5 << 11) | (g6 << 5) | b5;
}

// Genera un color RGB565 aleatori.
int random_color() {
    return rgb_to_565(rand() % 256, rand() % 256, rand() % 256);
}

// Pinta tota la pantalla amb el color indicat
void clear_screen(int color) {
    draw_rectangle_filled(0, 0, Size_X, Size_Y, color);
}


/**************** Calibració touchpad-pantalla **********************/
void touch_to_screen(uint16_t touch_x, uint16_t touch_y, int *screen_x, int *screen_y) {
    int32_t sx = (CAL_A * (int32_t)touch_x + CAL_B * (int32_t)touch_y + CAL_C) >> 16;
    int32_t sy = (CAL_D * (int32_t)touch_x + CAL_E * (int32_t)touch_y + CAL_F) >> 16;

    // Clamp a l'àrea de pantalla
    if (sx < 0)      sx = 0;
    if (sx > Size_X) sx = Size_X;
    if (sy < 0)      sy = 0;
    if (sy > Size_Y) sy = Size_Y;

    if (sx < NO_TOUCH_THRESHOLD_X || sy < NO_TOUCH_THRESHOLD_Y) {
        sx = -1;
        sy = -1;
    }

    *screen_x = (int)sx;
    *screen_y = (int)sy;
}
