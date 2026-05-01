#include "config.h"
#include <xc.h>

void __attribute__ ((interrupt,auto_psv)) _ADC1Interrupt(void) {
  LATA = (ADC1BUF0 >> 6) & 0x0F; // Show 4 MSB of ADCValue in PORTA
  IFS0bits.AD1IF = 0;            // Clear ADC interrupt flag
}

int main (void) {
  TRISA   = 0x0000; // PORTA as output
  AD1PCFG = 0xFFFB; // AN2 as analog, all other pins are digital
  AD1CHS  = 0x0002; // Connect RB0/AN2 as CH0 input
   
  AD1CON1bits.ADON   = 0; // Disable ADC
  AD1CON1bits.ADSIDL = 0; // Continue module operation in Idle mode
  AD1CON1bits.FORM   = 0; // Integer
  AD1CON1bits.SSRC   = 7; // Internal counter ends sampling and starts converting (auto-convert)
  AD1CON1bits.ASAM   = 0; // Sampling begins when SAMP bit is set
  AD1CON1bits.SAMP   = 0; // A/D sample/hold amplifier is holding
   
  AD1CON2bits.VCFG   = 0; // Reference voltage is AVdd, AVss
  AD1CON2bits.OFFCAL = 0; // Converts to get the actual input value
  AD1CON2bits.CSCNA  = 0; // Do not scan inputs
  AD1CON2bits.SMPI   = 0; // Interrupts at the completion for each sample/convert sequence
   
  AD1CON3bits.ADRC =  0; // Clock derived from system clock
  AD1CON3bits.SAMC = 31; // Sample time = 31 Tad
  AD1CON3bits.ADCS =  0; // Tad = 2*Tcy

  IPC3bits.AD1IP = 7;    // Interrupt priority
  IFS0bits.AD1IF = 0;    // Clear ADC interrupt flag
  IEC0bits.AD1IE = 1;    // Enable ADC interrupts
  
  AD1CON1bits.ADON = 1;  // Turn ADC on
  AD1CON1bits.ASAM = 1;  // Start sampling

  while (1) {
    LATAbits.LATA6 = ~PORTAbits.RA6;
  }
}
