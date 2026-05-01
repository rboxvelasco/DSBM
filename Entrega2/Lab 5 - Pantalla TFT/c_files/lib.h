#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "driver.h"
#include "colors.h"
#include "ascii5x7.h"

#ifndef LIB_H
#define LIB_H


/*************** CONSTANTS I AUXILIARS *****************************************/

// Dimensions de la pantalla
#define Orig_X  0
#define Orig_Y  0
#define Size_X  240
#define Size_Y  320

// Used in the draw_char and draw_text functions
#define FONT_WIDTH  5
#define FONT_HEIGHT 7

// Used in the draw_char_scaled and draw_text_scaled functions
typedef struct {
    const unsigned char *data;
    int width;
    int height;
} Font;


/*************** INICIALITZACIO ************************************************/
void init();
void quit();


/************* FUNCIONS BÀSIQUES *************************************************/
void draw_pixel(int x, int y, int color);
void draw_hline(int x, int y, int length, int color);
void draw_vline(int x, int y, int length, int color);


/************* FORMES GEOMETRIQUES *************************************************/
void draw_rectangle(int x, int y, int width, int height, int color);
void draw_filled_rectangle(int x, int y, int width, int height, int color);
void draw_white_rectangle(int x, int y, int width, int height);
void draw_white_filled_rectangle(int x, int y, int width, int height);


/***************** TEXT *****************************************************/
void draw_char(int x, int y, unsigned char c, int color);
void draw_text(int x, int y, const char *text, int color);
void draw_char_scaled(int x, int y, unsigned char c, int color, Font font, int scale);
void draw_text_scaled(int x, int y, const char *text, int color, Font font, int scale);


/*************** IMATGES ***************************************************/
void draw_image_file(int x, int y, int dest_w, int dest_h, const char *path);


/*************** UTILITATS ************************************************/
int  rgb_to_565(int r, int g, int b);
int  random_color();
void clear_screen(int color);

#endif // LIB_H
