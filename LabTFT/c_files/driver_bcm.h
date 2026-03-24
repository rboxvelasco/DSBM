// TFT Driver using bcm2835 native SPI
// Based on original driver by Enric X. Martin Rull (FIB, DSBM, March 2016)
// Adapted to bcm2835 SPI by — March 2026

#include <bcm2835.h>
#include <stdint.h>

// ---------------------------------------------
//  SPI Interface Functions
// ---------------------------------------------

void Config_Pins();

void Free_Pins();

// ---------------------------------------------
//  Reset line helper (still bit-banged, it is
//  not part of the SPI bus)
// ---------------------------------------------

void Reset_TFT(int value);

// ---------------------------------------------
//  SPI Level Functions
// ---------------------------------------------

// Send a command register index to the TFT.
// Protocol: 0x70 | SPI_WR | SPI_INDEX  →  reg (8-bit)
void Write_SPI_TFT_Cmd(int reg);

// Send a 16-bit data word to the TFT.
// Protocol: 0x72 | SPI_WR | SPI_DATA  →  high byte  →  low byte
void Write_SPI_TFT_Dat(int value);

// Write a 16-bit value to a TFT register.
void Write_SPI_TFT_Reg(int reg, int value);

// Read a TFT register (full read not implemented in original either).
int Read_SPI_TFT_Reg(int reg);

// ---------------------------------------------
//  TFT Level Functions
// ---------------------------------------------

void SPI_TFT_Reset();

void SPI_TFT_pixel(int x, int y, int color);

void SPI_TFT_region(int x1, int y1, int x2, int y2, int color);