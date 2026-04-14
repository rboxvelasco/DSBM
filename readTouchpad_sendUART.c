#include "config.h"
#include <xc.h>

#define FCY 4000000
#include <libpic30.h>

volatile char rx = '1';

void configure2readX() {
    // X+: AN3 -> RB1: output digital, valor 1
    TRISBbits.TRISB1 = 0;   // output
    ANSBbits.ANSB1   = 0;   // digital
    LATBbits.LATB1   = 1;   // valor lógico 1

    // X-: AN2 -> RB0: output digital, valor 0
    TRISBbits.TRISB0 = 0;   // output
    ANSBbits.ANSB0   = 0;   // digital
    LATBbits.LATB0   = 0;   // valor lógico 0

    // Y+: AN11 -> RB13: input digital
    TRISBbits.TRISB13 = 1;  // input
    ANSBbits.ANSB13   = 0;  // digital

    // Y-: AN10 -> RB14: input analógico
    TRISBbits.TRISB14 = 1;  // input
    ANSBbits.ANSB14   = 1;  // analógico
}

void configure2readY() {
    // Y+: AN11 -> RB13: output digital, valor 1
    TRISBbits.TRISB13 = 0;   // output
    ANSBbits.ANSB13   = 0;   // digital
    LATBbits.LATB13   = 1;   // valor lógico 1

    // Y-: AN10 -> RB14: output digital, valor 0
    TRISBbits.TRISB14 = 0;   // output
    ANSBbits.ANSB14   = 0;   // digital
    LATBbits.LATB14   = 0;   // valor lógico 0

    // X+: AN3 -> RB1: input digital
    TRISBbits.TRISB1 = 1;  // input
    ANSBbits.ANSB1   = 0;  // digital

    // X-: AN2 -> RB0: input analógico
    TRISBbits.TRISB0 = 1;  // input
    ANSBbits.ANSB0   = 1;  // analógico
}

void configure_UART() {

    asm volatile    ("MOV #OSCCON, w1   \n"
                  "MOV #0x46, w2      \n"
                  "MOV #0x57, w3      \n"
                  "MOV.b w2, [w1]     \n"
                  "MOV.b w3, [w1]     \n"
                  "BSET OSCCON, #6");


  /***********************************************/
  // Baud rate (9600)
  /***********************************************/
  U1MODEbits.BRGH = 1;
  U1BRG = 103;

  /***********************************************/
  // 8N1
  /***********************************************/
  U1MODEbits.RXINV = 0;
  U1MODEbits.PDSEL = 0;
  U1MODEbits.STSEL = 0;

  /***********************************************/
  // Enable UART
  /***********************************************/
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;

  /***********************************************/
  // RX Interrupt
  /***********************************************/
  IEC0bits.U1RXIE = 1;
  IFS0bits.U1RXIF = 0;
  IPC2bits.U1RXIP = 4;
  
  /***********************************************/
  // 1. Initialize the UxBRG register for the appropriate baud rate
  /***********************************************/
  // BRGH: High Baud Rate Select bit
  // 1 = High speed
  // 0 = Low speed
  U1MODEbits.BRGH = 1;

  // PIC24F Section 21 UART. Table 21-1
  // BRGH=0, Fcy=4MHz
  //
  //  Baud   Actual  %Error   BRG
  //   110    110.0    0.00  2272
  //   300    300.1    0.00   832
  //  1200   1201.9    0.16   207
  //  2400   2403.8    0.15   103
  //  9600   9615.4    0.20    25
  // 19.2k  19230.8    0.20    12

  // PIC24F Section 21 UART. Table 21-1
  // BRGH=1, Fcy=4MHz
  //
  //  Baud   Actual  %Error   BRG
  //   110    110.0    0.00  9090
  //   300    300.0    0.00  3332
  //  1200   1200.5    0.00   832
  //  2400   2398.1   -0.07   416
  //  9600   9615.3    0.16   103
  // 19.2k  19230.7    0.16    51
  // 38.4k  38461.5    0.16    25
  // 56.0k  55555.5   -0.79    17
  U1BRG = 103;

  /***********************************************/
  // 2. Set the number of data bits, number of Stop bits and parity selection
  /***********************************************/
  // RXINV: Receive Polarity Inversion bit
  // 1 = UxRX Idle state is '0'
  // 0 = UxRX Idle state is '1'
  U1MODEbits.RXINV = 0x0;

  // PDSEL<1:0>: Parity and Data Selection bits
  // 11 = 9-bit data, no parity
  // 10 = 8-bit data, odd parity
  // 01 = 8-bit data, even parity
  // 00 = 8-bit data, no parity
  U1MODEbits.PDSEL = 0x0;

  // STSEL: Stop Selection bit
  // 1 = 2 Stop bits
  // 0 = 1 Stop bit
  U1MODEbits.STSEL = 0x0;

  /***********************************************/
  // 3. If transmit interrupts are desired...
  /***********************************************/
  // Not implemented

  /***********************************************/
  // 4. Enable the UART module by setting the UARTEN
  /***********************************************/
  // UARTEN: UARTx Enable bit
  // 1 = UARTx is enabled; UARTx pins are controlled by UARTx as defined by UEN<1:0> and UTXEN control bits
  // 0 = UARTx is disabled; UARTx pins are controlled by corresponding PORT, LAT and TRIS bits
  U1MODEbits.UARTEN = 1;

  /***********************************************/
  // 5. Enable the transmission by setting the UTXEN
  /***********************************************/
  // UTXEN: Transmit Enable bit
  // 1 = UARTx transmitter enabled, UxTX pin controlled by UARTx (if UARTEN = 1)
  // 0 = UARTx transmitter disabled, any pending transmission is aborted and buffer is reset. UxTX pin controlled by PORT.
  U1STAbits.UTXEN = 1;
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
    AD1CON3 = 0x8010;

    // AD1CSSL: sin scan
    AD1CSSL = 0x0000;

    // Encender el ADC
    AD1CON1bits.ADON = 1;
}

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) {

  /* TODO

    while (U1STAbits.URXDA) {
        rx = U1RXREG;
    }
    
    if (old_state) button = '1';
    else button =  '0';
    
    IFS0bits.U1RXIF = 0;
    
    while (U1STAbits.UTXBF);
    U1TXREG = button;
    */
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

void send2bytes(uint16_t number) {
    uint8_t  highByte = (number >> 8) & 0xFF;  // byte alto
    uint8_t  lowByte  = number & 0xFF;         // byte bajo

    // Enviar byte alto
    while (U1STAbits.UTXBF);
    U1TXREG = highByte;

    // Enviar byte bajo
    while (U1STAbits.UTXBF);
    U1TXREG = lowByte;
}

int main(void) {

    configure_ADC();
    configure_UART();

    // Activar interrupciones globales (CLAVE)
    __builtin_enable_interrupts();

    while (1) {
      configure2readX();
      uint16_t x = readCoordX();
      configure2readX();
      uint16_t y = readCoordY();
      send2bytes(x);
      send2bytes(y);
      __delay_ms(2000);
      // while(!receeive ack)
    }
}
