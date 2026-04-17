#include <stdio.h>
#include <time.h>
#include "lib.h"
#include "colors.h"

Font Font5x7_struct = { Font5x7, 5, 7 };

int main() {
    printf("Call to config_pins\n");
    init();
    printf("Call to fill_screen:\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    clear_screen(GREEN);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Pantalla limpia, ejecutando draw_text_scaled...\n");
    draw_text_scaled(10, 10, "HOLA MUNDO :)", RED, Font5x7_struct, 2);
    draw_filled_rectangle(50, 50, 60, 60, BLUE);
    draw_image_file(30,100,160,200,"../images/on.png");
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Temps: %.6f segons\n", elapsed);
    return 0;
}

