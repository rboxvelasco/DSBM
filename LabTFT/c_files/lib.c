// lib.c -- TFT Graphics Library for Raspberry Pi + HX8347 SPI TFT (240x320)
// Requires: driver (SPI_TFT_pixel, SPI_TFT_Reset), font5x7.h, colors.h
// Build: gcc -o main main.c lib.c -lwiringPi -lm

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ascii5x7.h"
#include "colors.h"

// --- Forward declarations (from driver) --------------------------------------
extern void SPI_TFT_pixel(int x, int y, int color);
extern void SPI_TFT_Reset(void);

// --- Screen dimensions -------------------------------------------------------
#define SCREEN_W  240
#define SCREEN_H  320

// --- Font constants (5 cols x 7 rows, 1 px gap) ------------------------------
#define FONT_W     5
#define FONT_H     7
#define FONT_GAP   1
#define CHAR_W    (FONT_W + FONT_GAP)   // 6 px per character slot
#define CHAR_H    (FONT_H + FONT_GAP)   // 8 px per character slot
#define FONT_FIRST 0x20                 // first ascii code in Font5x7

// =============================================================================
//  1. SCREEN
// =============================================================================

/* Fill the entire screen with one color. */
void TFT_fill_screen(int color)
{
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
            SPI_TFT_pixel(x, y, color);
}

/* Clear the screen to black. */
void TFT_clear(void)
{
    TFT_fill_screen(BLACK);
}

// =============================================================================
//  2. LINES
// =============================================================================

/* Bresenham line between (x0,y0) and (x1,y1). */
void TFT_draw_line(int x0, int y0, int x1, int y1, int color)
{
    int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        SPI_TFT_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Horizontal line -- faster than the general case. */
void TFT_draw_hline(int x, int y, int w, int color)
{
    int i;
    for (i = 0; i < w; i++) SPI_TFT_pixel(x + i, y, color);
}

/* Vertical line. */
void TFT_draw_vline(int x, int y, int h, int color)
{
    int i;
    for (i = 0; i < h; i++) SPI_TFT_pixel(x, y + i, color);
}

/* Thick line -- draws 'thickness' parallel lines. */
void TFT_draw_thick_line(int x0, int y0, int x1, int y1, int color, int thickness)
{
    int t;
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int half = thickness / 2;
    for (t = -half; t <= half; t++) {
        if (dx >= dy)
            TFT_draw_line(x0, y0 + t, x1, y1 + t, color);
        else
            TFT_draw_line(x0 + t, y0, x1 + t, y1, color);
    }
}

// =============================================================================
//  3. RECTANGLES
// =============================================================================

/* Outlined rectangle. */
void TFT_draw_rect(int x, int y, int w, int h, int color)
{
    TFT_draw_hline(x,         y,         w, color);
    TFT_draw_hline(x,         y + h - 1, w, color);
    TFT_draw_vline(x,         y,         h, color);
    TFT_draw_vline(x + w - 1, y,         h, color);
}

/* Filled rectangle. */
void TFT_fill_rect(int x, int y, int w, int h, int color)
{
    int j;
    for (j = 0; j < h; j++)
        TFT_draw_hline(x, y + j, w, color);
}

/* Rounded-corner rectangle (outline). r = corner radius. */
void TFT_draw_round_rect(int x, int y, int w, int h, int r, int color)
{
    int cx, cy, f, ddF_x, ddF_y;
    int x1 = x + r, x2 = x + w - 1 - r;
    int y1 = y + r, y2 = y + h - 1 - r;

    /* Straight edges */
    TFT_draw_hline(x1, y,         w - 2*r, color);
    TFT_draw_hline(x1, y + h - 1, w - 2*r, color);
    TFT_draw_vline(x,         y1, h - 2*r, color);
    TFT_draw_vline(x + w - 1, y1, h - 2*r, color);

    /* Four corners -- midpoint circle quarter arcs */
    cx = 0; cy = r;
    f = 1 - r; ddF_x = 1; ddF_y = -2 * r;
    while (cx <= cy) {
        SPI_TFT_pixel(x2 + cx, y1 - cy, color); /* top-right    */
        SPI_TFT_pixel(x2 + cy, y1 - cx, color);
        SPI_TFT_pixel(x1 - cx, y1 - cy, color); /* top-left     */
        SPI_TFT_pixel(x1 - cy, y1 - cx, color);
        SPI_TFT_pixel(x2 + cx, y2 + cy, color); /* bottom-right */
        SPI_TFT_pixel(x2 + cy, y2 + cx, color);
        SPI_TFT_pixel(x1 - cx, y2 + cy, color); /* bottom-left  */
        SPI_TFT_pixel(x1 - cy, y2 + cx, color);
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x;
    }
}

/* Filled rounded rectangle. */
void TFT_fill_round_rect(int x, int y, int w, int h, int r, int color)
{
    int cx = 0, cy = r, f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    int x1 = x + r, x2 = x + w - 1 - r;
    int y1 = y + r, y2 = y + h - 1 - r;

    TFT_fill_rect(x + r, y, w - 2*r, h, color);
    while (cx <= cy) {
        TFT_draw_hline(x1 - cx, y1 - cy, cx*2 + (x2 - x1) + 1, color);
        TFT_draw_hline(x1 - cx, y2 + cy, cx*2 + (x2 - x1) + 1, color);
        TFT_draw_hline(x1 - cy, y1 - cx, cy*2 + (x2 - x1) + 1, color);
        TFT_draw_hline(x1 - cy, y2 + cx, cy*2 + (x2 - x1) + 1, color);
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x;
    }
}

// =============================================================================
//  4. CIRCLES & ELLIPSES
// =============================================================================

/* Outlined circle (Bresenham midpoint algorithm). */
void TFT_draw_circle(int cx, int cy, int r, int color)
{
    int x = 0, y = r, f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    SPI_TFT_pixel(cx,     cy + r, color);
    SPI_TFT_pixel(cx,     cy - r, color);
    SPI_TFT_pixel(cx + r, cy,     color);
    SPI_TFT_pixel(cx - r, cy,     color);
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        SPI_TFT_pixel(cx + x, cy + y, color);
        SPI_TFT_pixel(cx - x, cy + y, color);
        SPI_TFT_pixel(cx + x, cy - y, color);
        SPI_TFT_pixel(cx - x, cy - y, color);
        SPI_TFT_pixel(cx + y, cy + x, color);
        SPI_TFT_pixel(cx - y, cy + x, color);
        SPI_TFT_pixel(cx + y, cy - x, color);
        SPI_TFT_pixel(cx - y, cy - x, color);
    }
}

/* Filled circle. */
void TFT_fill_circle(int cx, int cy, int r, int color)
{
    int x = 0, y = r, f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    TFT_draw_hline(cx - r, cy, 2*r + 1, color);
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        TFT_draw_hline(cx - x, cy + y, 2*x + 1, color);
        TFT_draw_hline(cx - x, cy - y, 2*x + 1, color);
        TFT_draw_hline(cx - y, cy + x, 2*y + 1, color);
        TFT_draw_hline(cx - y, cy - x, 2*y + 1, color);
    }
}

/* Ring (donut) -- filled annulus from inner radius ri to outer radius ro. */
void TFT_draw_ring(int cx, int cy, int ro, int ri, int color)
{
    int x, y;
    int ro2 = ro*ro, ri2 = ri*ri;
    for (y = -ro; y <= ro; y++)
        for (x = -ro; x <= ro; x++) {
            int d2 = x*x + y*y;
            if (d2 <= ro2 && d2 >= ri2)
                SPI_TFT_pixel(cx + x, cy + y, color);
        }
}

/* Outlined ellipse. rx = horizontal radius, ry = vertical radius. */
void TFT_draw_ellipse(int cx, int cy, int rx, int ry, int color)
{
    long rx2 = (long)rx*rx, ry2 = (long)ry*ry;
    long twoRx2 = 2*rx2, twoRy2 = 2*ry2;
    long p, px, py;
    int x = 0, y = ry;

    p  = (long)(ry2 - rx2*ry + 0.25*rx2);
    px = 0;
    py = twoRx2 * y;

    while (px < py) {
        SPI_TFT_pixel(cx+x, cy+y, color); SPI_TFT_pixel(cx-x, cy+y, color);
        SPI_TFT_pixel(cx+x, cy-y, color); SPI_TFT_pixel(cx-x, cy-y, color);
        x++;
        px += twoRy2;
        if (p < 0) p += ry2 + px;
        else { y--; py -= twoRx2; p += ry2 + px - py; }
    }
    p = (long)(ry2*(x+0.5)*(x+0.5) + rx2*(y-1)*(y-1) - rx2*ry2);
    while (y >= 0) {
        SPI_TFT_pixel(cx+x, cy+y, color); SPI_TFT_pixel(cx-x, cy+y, color);
        SPI_TFT_pixel(cx+x, cy-y, color); SPI_TFT_pixel(cx-x, cy-y, color);
        y--;
        py -= twoRx2;
        if (p > 0) p += rx2 - py;
        else { x++; px += twoRy2; p += rx2 - py + px; }
    }
}

// =============================================================================
//  5. TRIANGLES
// =============================================================================

/* Outlined triangle. */
void TFT_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color)
{
    TFT_draw_line(x0, y0, x1, y1, color);
    TFT_draw_line(x1, y1, x2, y2, color);
    TFT_draw_line(x2, y2, x0, y0, color);
}

/* Filled triangle using scanline rasterization. */
void TFT_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color)
{
    int tmp, y, xa, xb;
#define SWAP(a,b) { tmp=(a); (a)=(b); (b)=tmp; }
    if (y0 > y1) { SWAP(y0,y1); SWAP(x0,x1); }
    if (y1 > y2) { SWAP(y1,y2); SWAP(x1,x2); }
    if (y0 > y1) { SWAP(y0,y1); SWAP(x0,x1); }
#undef SWAP

    for (y = y0; y <= y2; y++) {
        if (y < y1) {
            xa = (y1 != y0) ? x0 + (x1-x0)*(y-y0)/(y1-y0) : x0;
            xb =              x0 + (x2-x0)*(y-y0)/(y2-y0);
        } else {
            xa = (y2 != y1) ? x1 + (x2-x1)*(y-y1)/(y2-y1) : x1;
            xb =              x0 + (x2-x0)*(y-y0)/(y2-y0);
        }
        if (xa > xb) { int t = xa; xa = xb; xb = t; }
        TFT_draw_hline(xa, y, xb - xa + 1, color);
    }
}

// =============================================================================
//  6. TEXT -- single character
// =============================================================================

/*
 * Draw one ASCII character at pixel (x,y).
 *   scale : 1 = 5x7 px, 2 = 10x14 px, etc.
 *   fg    : foreground (glyph) color.
 *   bg    : background color; pass -1 for transparent background.
 */
void TFT_draw_char(int x, int y, char c, int scale, int fg, int bg)
{
    int col, row;
    if (c < FONT_FIRST || c > 0x7E) c = '?';
    int idx = (c - FONT_FIRST) * FONT_W;

    for (col = 0; col < FONT_W; col++) {
        unsigned char column = Font5x7[idx + col];
        for (row = 0; row < FONT_H; row++) {
            int bit = (column >> row) & 0x01;
            int px  = x + col * scale;
            int py  = y + row * scale;
            if (bit) {
                if (scale == 1) SPI_TFT_pixel(px, py, fg);
                else            TFT_fill_rect(px, py, scale, scale, fg);
            } else if (bg >= 0) {
                if (scale == 1) SPI_TFT_pixel(px, py, bg);
                else            TFT_fill_rect(px, py, scale, scale, bg);
            }
        }
    }
}

// =============================================================================
//  7. TEXT -- strings
// =============================================================================

/*
 * Draw a null-terminated string starting at (x,y).
 * Wraps to the next line when it exceeds SCREEN_W.
 * Supports '\n' for explicit line breaks.
 */
void TFT_draw_string(int x, int y, const char *s, int scale, int fg, int bg)
{
    int cx = x, cy = y;
    while (*s) {
        if (*s == '\n') {
            cx  = x;
            cy += CHAR_H * scale;
        } else {
            TFT_draw_char(cx, cy, *s, scale, fg, bg);
            cx += CHAR_W * scale;
            if (cx + CHAR_W * scale > SCREEN_W) {
                cx  = x;
                cy += CHAR_H * scale;
            }
        }
        s++;
    }
}

/* Draw a string centred horizontally at row y. */
void TFT_draw_string_centered(int y, const char *s, int scale, int fg, int bg)
{
    int len     = strlen(s);
    int total_w = len * CHAR_W * scale - FONT_GAP * scale;
    int x       = (SCREEN_W - total_w) / 2;
    if (x < 0) x = 0;
    TFT_draw_string(x, y, s, scale, fg, bg);
}

/* Draw an integer value as decimal text at (x,y). */
void TFT_draw_int(int x, int y, int value, int scale, int fg, int bg)
{
    char buf[16];
    int  i = 0, neg = 0;
    if (value < 0) { neg = 1; value = -value; }
    if (value == 0) buf[i++] = '0';
    while (value > 0) { buf[i++] = '0' + (value % 10); value /= 10; }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';
    /* reverse digits */
    int l = 0, r = i - 1;
    while (l < r) { char t = buf[l]; buf[l] = buf[r]; buf[r] = t; l++; r--; }
    TFT_draw_string(x, y, buf, scale, fg, bg);
}

/*
 * Scrolling marquee: draws 'text' sliding left inside a horizontal band.
 *   band_x, band_y : top-left of the band area.
 *   band_w, band_h : width and height of the band.
 *   offset         : current pixel scroll offset (start at 0).
 * Returns the next offset value (caller manages frame timing).
 *
 * Usage example:
 *   int off = 0;
 *   while (1) {
 *       off = TFT_marquee_step(0,150,240,16,"Hello!",off,1,WHITE,BLACK);
 *       delay(20);
 *   }
 */
int TFT_marquee_step(int band_x, int band_y, int band_w, int band_h,
                     const char *text, int offset, int scale, int fg, int bg)
{
    int len     = strlen(text);
    int total_w = len * CHAR_W * scale;
    int i, cx;

    TFT_fill_rect(band_x, band_y, band_w, band_h, bg);

    for (i = 0; i < len; i++) {
        cx = band_x + i * CHAR_W * scale - offset;
        if (cx + CHAR_W * scale < band_x) continue;
        if (cx >= band_x + band_w)         break;
        TFT_draw_char(cx, band_y, text[i], scale, fg, bg);
    }

    offset++;
    if (offset >= total_w + band_w) offset = 0;
    return offset;
}

// =============================================================================
//  8. UI WIDGETS
// =============================================================================

/*
 * Horizontal progress bar.
 *   value     : current value (0..max).
 *   bar_color : fill color.
 *   bg        : empty area color.
 *   border    : outline color.
 */
void TFT_progress_bar(int x, int y, int w, int h,
                      int value, int max,
                      int bar_color, int bg, int border)
{
    int fill;
    if (max <= 0) max = 1;
    fill = (value * (w - 2)) / max;
    TFT_fill_rect(x + 1, y + 1, w - 2, h - 2, bg);
    if (fill > 0)
        TFT_fill_rect(x + 1, y + 1, fill, h - 2, bar_color);
    TFT_draw_rect(x, y, w, h, border);
}

/*
 * Battery icon at (x,y), w x h px.
 * level = 0..100 (percent).
 * Color changes: green > 50%, yellow > 20%, red otherwise.
 */
void TFT_battery_icon(int x, int y, int w, int h, int level)
{
    int nub   = 3;
    int nub_h = h / 3;
    int fill_w, col;

    TFT_draw_rect(x, y, w - nub, h, WHITE);
    TFT_fill_rect(x + w - nub, y + (h - nub_h)/2, nub, nub_h, WHITE);
    fill_w = ((w - nub - 2) * level) / 100;
    col    = (level > 50) ? GREEN : (level > 20) ? YELLOW : RED;
    if (fill_w > 0)
        TFT_fill_rect(x + 1, y + 1, fill_w, h - 2, col);
}

/*
 * Labelled button.
 *   pressed = 0 normal, 1 pushed (no drop-shadow).
 */
void TFT_button(int x, int y, int w, int h,
                const char *label, int bg, int fg, int border, int pressed)
{
    int lw, lx, ly;
    if (!pressed)
        TFT_fill_rect(x + 2, y + 2, w, h, border); /* drop shadow */
    TFT_fill_rect(x, y, w, h, bg);
    TFT_draw_rect(x, y, w, h, border);
    lw = strlen(label) * CHAR_W;
    lx = x + (w - lw) / 2;
    ly = y + (h - CHAR_H) / 2;
    TFT_draw_string(lx, ly, label, 1, fg, bg);
}

/*
 * Vertical slider (thermometer style).
 * value = 0..max; fills from the bottom upwards.
 */
void TFT_vslider(int x, int y, int w, int h,
                 int value, int max, int fill_color, int bg, int border)
{
    int fill_h;
    if (max <= 0) max = 1;
    fill_h = ((h - 2) * value) / max;
    TFT_fill_rect(x + 1, y + 1, w - 2, h - 2, bg);
    if (fill_h > 0)
        TFT_fill_rect(x + 1, y + h - 1 - fill_h, w - 2, fill_h, fill_color);
    TFT_draw_rect(x, y, w, h, border);
}

/*
 * Line chart: plots an array of integer samples inside a framed box.
 *   data    : array of 'count' integer samples.
 *   min_val : value that maps to the bottom of the chart area.
 *   max_val : value that maps to the top.
 *   bg      : background color (-1 to skip clearing the box).
 */
void TFT_line_chart(int x, int y, int w, int h,
                    const int *data, int count,
                    int min_val, int max_val,
                    int line_color, int bg)
{
    int i, prev_px, prev_py, px, py, range;

    if (bg >= 0) TFT_fill_rect(x, y, w, h, bg);
    TFT_draw_rect(x, y, w, h, WHITE);

    if (count < 2) return;
    range = max_val - min_val;
    if (range == 0) range = 1;

    prev_px = x + 1;
    prev_py = y + h - 1 - (data[0] - min_val) * (h - 2) / range;

    for (i = 1; i < count; i++) {
        px = x + 1 + i * (w - 2) / (count - 1);
        py = y + h - 1 - (data[i] - min_val) * (h - 2) / range;
        TFT_draw_line(prev_px, prev_py, px, py, line_color);
        prev_px = px;
        prev_py = py;
    }
}

// =============================================================================
//  9. COLOR UTILITIES
// =============================================================================

/* Pack R,G,B (0-255 each) into RGB565. */
int TFT_rgb(int r, int g, int b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/* Unpack RGB565 into r, g, b (0-255). */
void TFT_unpack_rgb(int color, int *r, int *g, int *b)
{
    *r = (color >> 8) & 0xF8;
    *g = (color >> 3) & 0xFC;
    *b = (color << 3) & 0xF8;
}

/* Linear interpolation between two RGB565 colors (t = 0..256). */
int TFT_lerp_color(int c0, int c1, int t)
{
    int r0, g0, b0, r1, g1, b1;
    TFT_unpack_rgb(c0, &r0, &g0, &b0);
    TFT_unpack_rgb(c1, &r1, &g1, &b1);
    return TFT_rgb(r0 + (r1-r0)*t/256,
                   g0 + (g1-g0)*t/256,
                   b0 + (b1-b0)*t/256);
}

// =============================================================================
//  10. GRADIENT FILLS
// =============================================================================

/* Horizontal gradient: c0 on the left, c1 on the right. */
void TFT_gradient_h(int x, int y, int w, int h, int c0, int c1)
{
    int i;
    for (i = 0; i < w; i++)
        TFT_draw_vline(x + i, y, h, TFT_lerp_color(c0, c1, i * 256 / w));
}

/* Vertical gradient: c0 at the top, c1 at the bottom. */
void TFT_gradient_v(int x, int y, int w, int h, int c0, int c1)
{
    int j;
    for (j = 0; j < h; j++)
        TFT_draw_hline(x, y + j, w, TFT_lerp_color(c0, c1, j * 256 / h));
}

/* Radial gradient centred at (cx,cy) with outer radius r. */
void TFT_gradient_radial(int cx, int cy, int r, int c_center, int c_edge)
{
    int x, y;
    for (y = cy - r; y <= cy + r; y++)
        for (x = cx - r; x <= cx + r; x++) {
            int dx = x - cx, dy = y - cy;
            int d2 = dx*dx + dy*dy;
            if (d2 <= r*r) {
                int d   = (int)sqrt((double)d2);
                int col = TFT_lerp_color(c_center, c_edge, d * 256 / r);
                SPI_TFT_pixel(x, y, col);
            }
        }
}

// =============================================================================
//  11. BITMAP (1-bit packed, MSB first)
// =============================================================================

/*
 * Draw a monochrome bitmap at (x,y).
 *   data : byte array; each bit = 1 pixel, MSB first, rows padded to bytes.
 *   fg   : color for set bits.
 *   bg   : color for clear bits; pass -1 for transparent background.
 */
void TFT_draw_bitmap(int x, int y, int w, int h,
                     const unsigned char *data, int fg, int bg)
{
    int row, col;
    int stride = (w + 7) / 8;
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            int byte_idx = row * stride + col / 8;
            int bit_idx  = 7 - (col % 8);
            int bit      = (data[byte_idx] >> bit_idx) & 1;
            if (bit)
                SPI_TFT_pixel(x + col, y + row, fg);
            else if (bg >= 0)
                SPI_TFT_pixel(x + col, y + row, bg);
        }
}

// =============================================================================
//  12. SPECIAL EFFECTS
// =============================================================================

/*
 * Checkerboard pattern.
 *   cell : side length of each square in pixels.
 */
void TFT_checkerboard(int x, int y, int w, int h, int cell, int c0, int c1)
{
    int cx, cy;
    for (cy = 0; cy < h; cy += cell)
        for (cx = 0; cx < w; cx += cell) {
            int col = (((cx/cell) + (cy/cell)) & 1) ? c1 : c0;
            int pw  = (cx + cell > w) ? w - cx : cell;
            int ph  = (cy + cell > h) ? h - cy : cell;
            TFT_fill_rect(x + cx, y + cy, pw, ph, col);
        }
}

/*
 * Star-field: fills the screen with black then draws 'n' random star pixels.
 * Call srand() before the first call for different patterns each run.
 */
void TFT_starfield(int n)
{
    int i;
    TFT_fill_screen(BLACK);
    for (i = 0; i < n; i++) {
        int x      = rand() % SCREEN_W;
        int y      = rand() % SCREEN_H;
        int bright = (rand() % 3 == 0) ? WHITE : GRAY;
        SPI_TFT_pixel(x, y, bright);
    }
}

/*
 * Plasma effect -- colourful animated colour pattern over a rectangle.
 *   t : frame counter incremented by the caller each frame.
 * Note: full-screen is slow over SPI; use a small region (e.g. 80x80).
 */
void TFT_plasma(int x, int y, int w, int h, int t)
{
    int px, py;
    for (py = 0; py < h; py++)
        for (px = 0; px < w; px++) {
            double fx = px * 0.1, fy = py * 0.1, ft = t * 0.05;
            double v  = sin(fx + ft)
                      + sin(fy + ft)
                      + sin((fx + fy) * 0.5 + ft)
                      + sin(sqrt(fx*fx + fy*fy) + ft);
            /* v in [-4, 4] -> map to RGB */
            int r = (int)((sin(v)         + 1.0) * 127.5);
            int g = (int)((sin(v + 2.094) + 1.0) * 127.5);
            int b = (int)((sin(v + 4.189) + 1.0) * 127.5);
            SPI_TFT_pixel(x + px, y + py, TFT_rgb(r, g, b));
        }
}

/*
 * XOR colour pattern -- retro look, fills a rectangle.
 *   offset : shift the pattern each frame for animation.
 */
void TFT_xor_pattern(int x, int y, int w, int h, int offset)
{
    int px, py;
    for (py = 0; py < h; py++)
        for (px = 0; px < w; px++) {
            int v = ((px + offset) ^ (py + offset)) & 0xFF;
            int r = (v << 1) & 0xFF;
            int g = (v << 2) & 0xFF;
            int b = v;
            SPI_TFT_pixel(x + px, y + py, TFT_rgb(r, g, b));
        }
}

/*
 * Sine-wave line: draws a coloured sine wave across a horizontal band.
 * Useful as an animated separator, header or footer decoration.
 *   y_center  : vertical centre of the wave.
 *   amplitude : peak deviation in pixels.
 *   freq      : full cycles across SCREEN_W (try 2 or 3).
 *   phase     : pixel offset for animation -- increment each frame.
 *   thickness : line thickness in pixels.
 */
void TFT_sine_wave(int y_center, int amplitude, double freq,
                   int phase, int color, int thickness)
{
    int x, t;
    for (x = 0; x < SCREEN_W; x++) {
        int y = y_center + (int)(amplitude *
                sin(2.0 * M_PI * freq * (x + phase) / SCREEN_W));
        for (t = -thickness/2; t <= thickness/2; t++)
            SPI_TFT_pixel(x, y + t, color);
    }
}

