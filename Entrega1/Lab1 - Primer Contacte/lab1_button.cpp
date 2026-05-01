#include "config.h"
#include <xc.h>

int main() {
  TRISBbits.TRISB0  = 0b0; // Set RB0 as output
  
  AD1PCFGbits.PCFG3 = 0b1; // Set RB1 as digital
  TRISBbits.TRISB1  = 0b1; // Set RB1 as input

  while (1) {
    LATBbits.LATB0 = PORTBbits.RB1;
  }
}
