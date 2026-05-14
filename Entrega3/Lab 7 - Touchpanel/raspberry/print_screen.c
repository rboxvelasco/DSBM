#include <stdio.h>
#include <time.h>
#include "lib_tft.h"

#include "colors.h"

Font Font5x7_struct = { Font5x7, 5, 7 };

int main() {
    printf("Cridant config_pins...\n");
    init_TFT();

    printf("Cridant fill_screen...\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    clear_screen(GREEN);
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Pantalla neta. Dibuixant rectangle, text i imatge...\n");
    draw_text(10, 10, "HOLA MUNDO :)", RED, Font5x7_struct, 2);
    draw_image(30,100,60,60,"../../../assets/on.png");
    draw_rectangle_filled(30, 100, 60, 60, BLUE);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Temps de rentat: %.6f segons\n", elapsed);
    return 0;
}

