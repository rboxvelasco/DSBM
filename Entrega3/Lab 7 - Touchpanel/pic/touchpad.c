#include "config.h"
#include <xc.h>

#define FCY 4000000
#include <libpic30.h>


// Variable global compartida amb la ISR
volatile char rx = '1';


/************   ANALOG READ **********************/

int is_touched() {
    // X+ (RB1) -> HIGH, X- (RB0) -> LOW
    TRISBbits.TRISB1 = 0; LATBbits.LATB1 = 1;
    TRISBbits.TRISB0 = 0; LATBbits.LATB0 = 0;

    // Y+ (RB13) -> input digital
    TRISBbits.TRISB13 = 1;
    AD1PCFGbits.PCFG11 = 1;

    __delay_us(50);
    return PORTBbits.RB13; // 1 = tocat, 0 = no tocat
}

void configure2readX() {
    // X+: AN3 -> RB1: output digital, valor 1
    TRISBbits.TRISB1 = 0;
    AD1PCFGbits.PCFG3 = 1; // digital
    LATBbits.LATB1 = 1;

    // X-: AN2 -> RB0: output digital, valor 0
    TRISBbits.TRISB0 = 0;
    AD1PCFGbits.PCFG2 = 1; // digital
    LATBbits.LATB0 = 0;

    // Y+: AN11 -> RB13: input digital
    TRISBbits.TRISB13 = 1;
    AD1PCFGbits.PCFG11 = 1; // digital

    // Y-: AN10 -> RB14: input analògic
    TRISBbits.TRISB14 = 1;
    AD1PCFGbits.PCFG10 = 0; // analògic
}

void configure2readY() {
    // Y+: AN11 -> RB13: output digital, valor 1
    TRISBbits.TRISB13 = 0;
    AD1PCFGbits.PCFG11 = 1; // digital
    LATBbits.LATB13 = 1;

    // Y-: AN10 -> RB14: output digital, valor 0
    TRISBbits.TRISB14 = 0;
    AD1PCFGbits.PCFG10 = 1; // digital
    LATBbits.LATB14 = 0;

    // X+: AN3 -> RB1: input digital
    TRISBbits.TRISB1 = 1;
    AD1PCFGbits.PCFG3 = 1; // digital

    // X-: AN2 -> RB0: input analògic
    TRISBbits.TRISB0 = 1;
    AD1PCFGbits.PCFG2 = 0; // analògic
}

void configure_ADC() {
    // AD1CON1: ADC apagado durante config
    //   SSRC<2:0> = 111  -> conversión automática por reloj interno
    //   ASAM = 0         -> muestreo manual (SAMP controlado por software)
    //   FORM<1:0> = 00   -> resultado entero sin signo (0..1023)
    AD1CON1 = 0x00E0;           // SSRC=111, resto a 0

    // AD1CON2: referencias VDD/VSS, sin scan, 1 muestra por interrupción
    AD1CON2 = 0x0000;

    // AD1CON3: reloj interno RC del ADC (ADRC=1), tiempo de muestreo = 16 TAD
    //   ADRC = 1  -> clock RC interno (independiente de Fcy)
    //   SAMC<4:0> = 10000 -> 16 TAD de muestreo automático
    //AD1CON3 = 0x801F;
    AD1CON3bits.ADRC =  0; // Clock derived from system clock
    AD1CON3bits.SAMC = 31; // Sample time = 31 Tad
    AD1CON3bits.ADCS =  0; // Tad = 2*Tcy

    AD1CSSL = 0x0000; // AD1CSSL: sin scan

    AD1CON1bits.ADON = 1; // Encender el ADC
}

static uint16_t readADC(uint8_t channel) {
    // Seleccionar canal (CH0SA<3:0>)
    AD1CHSbits.CH0SA = channel;
    AD1CHSbits.CH0NA = 0;       // negativo a VREF- (GND)

    AD1CON1bits.DONE = 0;       // limpiar flag
    AD1CON1bits.SAMP = 1;       // iniciar muestreo
    // Con SSRC=111 y SAMC=16, la conversión arranca sola tras 16 TAD
    while (!AD1CON1bits.DONE);  // esperar fin de conversión

    return ADC1BUF0;            // resultado 10 bits (0..1023)
}

uint16_t readCoordX() {
    return readADC(10);         // CH0SA = 10 -> AN10
}

uint16_t readCoordY() {
    return readADC(2);          // CH0SA = 2  -> AN2
}

/***********   UART   *************************/

void configure_UART() {

    asm volatile("MOV #OSCCON, w1   \n"
                 "MOV #0x46, w2     \n"
                 "MOV #0x57, w3     \n"
                 "MOV.b w2, [w1]    \n"
                 "MOV.b w3, [w1]    \n"
                 "BSET OSCCON, #6"
                );


  // Baud rate (9600)
  U1MODEbits.BRGH = 1;
  U1BRG = 103;

  // 8N1
  U1MODEbits.RXINV = 0;
  U1MODEbits.PDSEL = 0;
  U1MODEbits.STSEL = 0;

  // Enable UART
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;

  // RX Interrupt
  IEC0bits.U1RXIE = 1;
  IFS0bits.U1RXIF = 0;
  IPC2bits.U1RXIP = 4;
}

// ISR UART RX
// Not used, és per rebre dades
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    while (U1STAbits.URXDA) {
        rx = U1RXREG;
    }
    IFS0bits.U1RXIF = 0;
}

void send_UART (char c) {
   while (U1STAbits.UTXBF);
   U1TXREG = c;
}

void send_coords(uint16_t x, uint16_t y) {
    while (U1STAbits.UTXBF); U1TXREG = (uint8_t)(x & 0xFF);        // X low
    while (U1STAbits.UTXBF); U1TXREG = (uint8_t)((x >> 8) & 0xFF); // X high
    while (U1STAbits.UTXBF); U1TXREG = (uint8_t)(y & 0xFF);        // Y low
    while (U1STAbits.UTXBF); U1TXREG = (uint8_t)((y >> 8) & 0xFF); // Y high
}



/***************   MAIN   ****************************/

int main(void) { 
  
  configure_UART();
  configure_ADC();
  uint16_t x, y;
  x = 256;
  
  // Activar interrupcions
  __builtin_enable_interrupts();
  
  while(1) {
      configure2readX();
      __delay_ms(10);
      x = readCoordX();
      configure2readY();
      y = readCoordY();
      
      //send_UART();
      send_coords(x, y);
      __delay_ms(5);
  }

  return 0;
}
