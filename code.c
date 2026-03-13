#include<stdio.h>
#include"driver.c"

int main() {
  printf("Call to config_pins\n");
  Config_Pins();
  printf("Call to reset:\n");
  SPI_TFT_Reset();

  while(1);
}
