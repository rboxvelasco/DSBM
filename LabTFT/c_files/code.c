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

    clear_screen(ORANGE);

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Pantalla limpia, ejecutando draw_text_scaled...\n");
//    draw_image_file(0,0,Size_X,Size_Y,"../images/on.png");
    draw_text_scaled(10, 10, "HOLA MUNDO :)", RED, Font5x7_struct, 2);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps: %.6f segons\n", elapsed);

    return 0;
}
