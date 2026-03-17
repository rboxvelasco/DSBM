#include <stdio.h>
#include <time.h>
#include "lib.c"
#include "driver.c"
#include "colors.h"

int main() {
    printf("Call to config_pins\n");
    Config_Pins();

    printf("Call to fill_screen:\n");

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    TFT_fill_screen(RED);
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps: %.6f segons\n", elapsed);

    return 0;
}
