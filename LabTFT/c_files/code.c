#include <stdio.h>
#include <time.h>
#include "lib.h"
#include "driver.h"
#include "colors.h"

Font Font5x7_struct = { Font5x7, 5, 7 };


int main() {
    printf("Call to config_pins\n");
    init();

    printf("Call to fill_screen:\n");

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    clear_screen(GREEN);
    draw_text_scaled(10,10,"HOLA :) MUNDO", RED, Font5x7_struct, 2);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps: %.6f segons\n", elapsed);

    return 0;
}
