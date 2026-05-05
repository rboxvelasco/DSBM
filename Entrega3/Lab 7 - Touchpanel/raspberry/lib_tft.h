#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "driver.h"
#include "colors.h"
#include "ascii5x7.h"
#include "stb_image.h"

#ifndef LIB_TFT_H
#define LIB_TFT_H

/*************** CONSTANTS I AUXILIARS *****************************************/

// Dimensions de la pantalla
#define Orig_X  0
#define Orig_Y  0
#define Size_X  240
#define Size_Y  320

// Calibració touchpad-pantalla 

#define CAL_A   25055    /*  0.382295 * 65536 */
#define CAL_B     783    /*  0.011948 * 65536 */
#define CAL_C  -4208630  /* -64.2113  * 65536 */

#define CAL_D     942    /*  0.014370 * 65536 */
#define CAL_E   37299    /*  0.569150 * 65536 */
#define CAL_F  -6079841  /* -92.7876  * 65536 */

// Used in the draw_char_scaled and draw_text_scaled functions
typedef struct {
    const unsigned char *data;
    int width;
    int height;
} Font;

/*************** INICIALITZACIO ************************************************/
void init_TFT();
void quit();


/************* FUNCIONS BÀSIQUES *************************************************/
void draw_pixel(int x, int y, int color);
void draw_pixel_scaled(int x, int y, int color, int scale);
void draw_hline(int x, int y, int length, int color);
void draw_vline(int x, int y, int length, int color);
void draw_line(int x0, int y0, int x1, int y1, int color);


/************* FORMES GEOMETRIQUES *************************************************/
void draw_rectangle(int x, int y, int width, int height, int color);
void draw_rectangle_filled(int x, int y, int width, int height, int color);
void draw_circle(int cx, int cy, int r, int color);
void draw_circle_filled(int cx, int cy, int r, int color);
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
void draw_triangle_filled(int x0, int y0, int x1, int y1, int x2, int y2, int color);


/***************** TEXT *****************************************************/
void draw_char(int x, int y, unsigned char c, int color, Font font, int scale);
void draw_text(int x, int y, const char *text, int color, Font font, int scale);


/*************** IMATGES ***************************************************/
void draw_image(int x, int y, int dest_w, int dest_h, const char *path);


/*************** UTILITATS ************************************************/
int  rgb_to_565(int r, int g, int b);
int  random_color();
void clear_screen(int color);
void touch_to_screen(uint16_t touch_x, uint16_t touch_y, int *screen_x, int *screen_y);

#endif // LIB_TFT_H
