
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "colors.h"
#include "driver.h"
#include "ascii5x7.h"


#ifndef LIB_H
#define LIB_H

// Dimensions de la pantalla
#define Orig_X  0
#define Orig_Y  0
#define Size_X  240
#define Size_Y  320

// Colors predefinits RGB565
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F


// Auxiliars per a escalar texts
typedef struct {
    const unsigned char *data;
    int width;
    int height;
} Font;

//Font Font5x7_struct = { Font5x7, 5, 7 };


// Inicialitzacio i alliberament
void init();
void quit();

// Primitives grafiques base
void draw_pixel(int x, int y, int color);
void draw_hline(int x, int y, int length, int color);
void draw_vline(int x, int y, int length, int color);

// Rectangles
void draw_rectangle(int x, int y, int width, int height, int color);
void draw_filled_rectangle(int x, int y, int width, int height, int color);
void draw_white_rectangle(int x, int y, int width, int height);
void draw_white_filled_rectangle(int x, int y, int width, int height);
void draw_char(int x, int y, char c, int color);
void draw_text(int x, int y, const char *text, int color);
void draw_char_scaled(int x, int y, char c, int color, Font font, int scale);
void draw_text_scaled(int x, int y, const char *text, int color, Font font, int scale);

// Utilitats
int  rgb_to_565(int r, int g, int b);
int  random_color();
void clear_screen(int color);

#endif // LIB_H
