// TFT Graphics Library
// Basada en el driver SPI d'Enric X. Martin Rull (FIB, DSBM, Març 2016)
// Llibreria de funcions grafiques d'alt nivell per a pantalla TFT 240x320
#include "lib.h"
#include "stb_image.h"

// Prototips interns del driver (definits a driver.c)
void Config_Pins();
void SPI_TFT_Reset();
void SPI_TFT_pixel(int x, int y, int color);

//     INICIALITZACIO

/**
 * Inicialitza els pins GPIO i la pantalla TFT.
 * Crida aquesta funcio una sola vegada a l'inici del programa.
 */
void init()
{
    Config_Pins();
    SPI_TFT_Reset();
    srand(time(NULL));
}

/**
 * Allibera els recursos GPIO.
 * Crida aquesta funcio en tancar el programa.
 */
void quit()
{
    // wiringPi no requereix cleanup explicit,
    // pero es pot afegir logica de tancament aqui si cal.
}


//     PRIMITIVES GRaFIQUES BASE

/**
 * Dibuixa un pixel a (x, y) amb el color RGB565 indicat.
 */
void draw_pixel(int x, int y, int color)
{
    SPI_TFT_pixel(x, y, color);
}

/**
 * Dibuixa una linia horitzontal.
 *
 * Parametres:
 *   x      -- coordenada X d'inici
 *   y      -- coordenada Y (fixa)
 *   length -- longitud en pixels
 *   color  -- color RGB565
 */
/*
void draw_hline(int x, int y, int length, int color)
{
    int i;
    for (i = 0; i < length; i++)
        draw_pixel(x + i, y, color);
}
*/


void draw_hline(int x, int y, int length, int color)
{
    // Usamos SPI_TFT_region para dibujar una línea horizontal completa
    SPI_TFT_region(x, y, x + length - 1, y, color);
}




/**
 * Dibuixa una linia vertical.
 *
 * Parametres:
 *   x      -- coordenada X (fixa)
 *   y      -- coordenada Y d'inici
 *   length -- longitud en pixels
 *   color  -- color RGB565
 */
/*
void draw_vline(int x, int y, int length, int color)
{
    int i;
    for (i = 0; i < length; i++)
        draw_pixel(x, y + i, color);
}
*/

void draw_vline(int x, int y, int length, int color)
{
    // Usamos SPI_TFT_region para dibujar una línea vertical completa
    SPI_TFT_region(x, y, x, y + length - 1, color);
}



//     RECTANGLES                                                                

/**
 * Dibuixa el contorn d'un rectangle.
 *
 * Parametres:
 *   x, y         -- cantonada superior esquerra
 *   width        -- amplada en pixels
 *   height       -- alçada en pixels
 *   color        -- color RGB565 (usa COLOR_WHITE, COLOR_RED, etc.)
 *
 * Exemple:
 *   draw_rectangle(10, 10, 100, 60, COLOR_WHITE);
 */
void draw_rectangle(int x, int y, int width, int height, int color)
{
    draw_hline(x,             y,              width,  color);  // costat superior
    draw_hline(x,             y + height - 1, width,  color);  // costat inferior
    draw_vline(x,             y,              height, color);  // costat esquerre
    draw_vline(x + width - 1, y,              height, color);  // costat dret
}

/**
 * Dibuixa un rectangle ple.
 *
 * Parametres:
 *   x, y         -- cantonada superior esquerra
 *   width        -- amplada en pixels
 *   height       -- alçada en pixels
 *   color        -- color RGB565
 *
 * Exemple:
 *   draw_filled_rectangle(10, 10, 100, 60, COLOR_WHITE);
 */

/*
void draw_filled_rectangle(int x, int y, int width, int height, int color)
{
    int row;
    for (row = 0; row < height; row++)
        draw_hline(x, y + row, width, color);
}
*/


void draw_filled_rectangle(int x, int y, int width, int height, int color)
{
    // Usamos SPI_TFT_region para dibujar el rectángulo completo
    SPI_TFT_region(x, y, x + width - 1, y + height - 1, color);
}



/**
 * Dibuixa el contorn d'un rectangle BLANC.
 *
 * Parametres:
 *   x, y         -- cantonada superior esquerra
 *   width        -- amplada en pixels
 *   height       -- alçada en pixels
 *
 * Exemple:
 *   draw_white_rectangle(20, 20, 80, 50);
 *   -> rectangle blanc de 80x50 px amb cantonada sup. esq. a (20,20)
 */
void draw_white_rectangle(int x, int y, int width, int height)
{
    draw_rectangle(x, y, width, height, WHITE);
}

/**
 * Dibuixa un rectangle BLANC ple.
 *
 * Parametres:
 *   x, y         -- cantonada superior esquerra
 *   width        -- amplada en pixels
 *   height       -- alçada en pixels
 *
 * Exemple:
 *   draw_white_filled_rectangle(20, 20, 80, 50);
 */
void draw_white_filled_rectangle(int x, int y, int width, int height)
{
    draw_filled_rectangle(x, y, width, height, WHITE);
}


void draw_char(int x, int y, unsigned char c, int color)
{
    if (c < 32 || c > 127) return;

    int index = (c - 32) * 5;

    for (int col = 0; col < FONT_WIDTH; col++)
    {
        unsigned char line = Font5x7[index + col];

        for (int row = 0; row < FONT_HEIGHT; row++)
        {
            if (line & (1 << row))
            {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}


void draw_char_scaled(int x, int y, unsigned char c, int color, Font font, int scale)
{
    if (c < 32 || c > 127) return;
    if (scale < 1) scale = 1;

    int index = (c - 32) * font.width;

    for (int col = 0; col < font.width; col++)
    {
        unsigned char line = font.data[index + col];

        for (int row = 0; row < font.height; row++)
        {
            if (line & (1 << row))
            {
                // dibuixa bloc escalat
                for (int dx = 0; dx < scale; dx++)
                {
                    for (int dy = 0; dy < scale; dy++)
                    {
                        draw_pixel(
                            x + col * scale + dx,
                            y + row * scale + dy,
                            color
                        );
                    }
                }
            }
        }
    }
}

void draw_text(int x, int y, const char *text, int color)
{
    int cursor_x = x;
    int cursor_y = y;

    while (*text)
    {
        // Si arribem a baix de la pantalla → nova columna
        if (cursor_y + FONT_HEIGHT >= Size_Y)
        {
            cursor_y = y;
            cursor_x += FONT_WIDTH + 1;
        }

        draw_char(cursor_x, cursor_y, *text, color);

        cursor_y += FONT_HEIGHT + 1; // avanç vertical
        text++;
    }
}

void draw_text_scaled(int x, int y, const char *text, int color, Font font, int scale) {
    if (scale < 1) scale = 1;

    int cursor_x = x;
    int cursor_y = y;
    int char_w = (font.width * scale) + scale;
    int char_h = (font.height * scale) + scale;

    while (*text)
    {
        // Salt de línia explícit '\n'
        if (*text == '\n')
        {
            cursor_x = x;
            cursor_y += char_h;
            text++;
            continue;
        }

        // Si el caràcter no cap, salt de línia automàtic
        if (cursor_x + char_w > Size_X)
        {
            cursor_x = x;
            cursor_y += char_h;
        }

        // Si sortim per baix de la pantalla, parem
        if (cursor_y + font.height * scale > Size_Y) break;

        draw_char_scaled(cursor_x, cursor_y, *text, color, font, scale);
        cursor_x += char_w;
        text++;
    }
}

void draw_image_file(int x, int y, int dest_w, int dest_h, const char *path)
{
    int src_w, src_h, channels;

    // Carrega la imatge i força 3 canals (RGB)
    unsigned char *data = stbi_load(path, &src_w, &src_h, &channels, 3);
    if (!data) {
        printf("Error: no s'ha pogut carregar '%s': %s\n",
               path, stbi_failure_reason());
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


//     UTILITATS

/**
 * Converteix un color RGB (0-255 per canal) a format RGB565.
 *
 * Parametres:
 *   r -- component vermell  (0-255)
 *   g -- component verd     (0-255)
 *   b -- component blau     (0-255)
 *
 * Retorna:
 *   Enter de 16 bits en format RGB565.
 *
 * Exemple:
 *   int blanc = rgb_to_565(255, 255, 255);  ->  0xFFFF
 *   int vermell = rgb_to_565(255, 0, 0);    ->  0xF800
 */
int rgb_to_565(int r, int g, int b)
{
    int r5 = (r >> 3) & 0x1F;
    int g6 = (g >> 2) & 0x3F;
    int b5 = (b >> 3) & 0x1F;
    return (r5 << 11) | (g6 << 5) | b5;
}

/**
 * Genera un color RGB565 aleatori.
 */
int random_color()
{
    return rgb_to_565(
        rand() % 256,
        rand() % 256,
        rand() % 256
    );
}

/**
 * Neteja tota la pantalla amb el color indicat (negre per defecte si passes COLOR_BLACK).
 *
 * Exemple:
 *   clear_screen(COLOR_BLACK);   // pantalla negra
 *   clear_screen(COLOR_WHITE);   // pantalla blanca
 */
void clear_screen(int color)
{
    draw_filled_rectangle(0, 0, Size_X, Size_Y, color);
}
