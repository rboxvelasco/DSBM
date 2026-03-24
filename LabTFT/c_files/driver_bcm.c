// TFT Driver using bcm2835 native SPI
// Based on original driver by Enric X. Martin Rull (FIB, DSBM, March 2016)
// Adapted to bcm2835 SPI by — March 2026

#include "driver.h"

// ---------------------------------------------
//  SPI Interface Functions
// ---------------------------------------------

void Config_Pins()
{
    if (!bcm2835_init()) return;

    bcm2835_spi_begin();

    // CS polarity: active low
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, 0);

    // Clock: ~1.953 MHz (divider 128) — safe for most TFT displays;
    // increase to BCM2835_SPI_CLOCK_DIVIDER_64 (3.9 MHz) if stable.
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);

    // SPI Mode 0: CPOL=0, CPHA=0 — matches the original bit-bang behaviour
    // (clock idle low, data sampled on rising edge)
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

    // Use CS0
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);

    // Reset pin: use a spare GPIO (same as original Reset_Pin)
    bcm2835_gpio_fsel(Reset_Pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(Reset_Pin, HIGH);

    bcm2835_delay(200);
}

void Free_Pins()
{
    bcm2835_spi_end();
    bcm2835_close();
}

// ---------------------------------------------
//  Reset line helper (still bit-banged, it is
//  not part of the SPI bus)
// ---------------------------------------------

void Reset_TFT(int value)
{
    bcm2835_gpio_write(Reset_Pin, value);
}

// ---------------------------------------------
//  SPI Level Functions
// ---------------------------------------------

// Send a command register index to the TFT.
// Protocol: 0x70 | SPI_WR | SPI_INDEX  →  reg (8-bit)
void Write_SPI_TFT_Cmd(int reg)
{
    uint8_t buf[2];
    buf[0] = 0x70;          // Start byte: write + index
    buf[1] = (uint8_t)reg;
    bcm2835_spi_transfern((char *)buf, 2);
}

// Send a 16-bit data word to the TFT.
// Protocol: 0x72 | SPI_WR | SPI_DATA  →  high byte  →  low byte
void Write_SPI_TFT_Dat(int value)
{
    uint8_t buf[3];
    buf[0] = 0x72;                      // Start byte: write + data
    buf[1] = (uint8_t)(value >> 8);     // High byte
    buf[2] = (uint8_t)(value & 0xFF);   // Low byte
    bcm2835_spi_transfern((char *)buf, 3);
}

// Write a 16-bit value to a TFT register.
void Write_SPI_TFT_Reg(int reg, int value)
{
    Write_SPI_TFT_Cmd(reg);
    Write_SPI_TFT_Dat(value);
}

// Read a TFT register (full read not implemented in original either).
int Read_SPI_TFT_Reg(int reg)
{
    Write_SPI_TFT_Cmd(reg);
    // TODO: implement 16-bit read if needed
    return 0x0000;
}

// ---------------------------------------------
//  TFT Level Functions
// ---------------------------------------------

void SPI_TFT_Reset()
{
    // Hardware reset
    Reset_TFT(0);
    bcm2835_delay(250);
    Reset_TFT(1);
    bcm2835_delay(6);

    int driverCode = Read_SPI_TFT_Reg(0x00);

    // Start Initial Sequence
    Write_SPI_TFT_Reg(0xEA, 0x0000);   // Reset Power Control 1
    Write_SPI_TFT_Reg(0xEB, 0x0020);   // Power Control 2
    Write_SPI_TFT_Reg(0xEC, 0x000C);   // Power Control 3
    Write_SPI_TFT_Reg(0xED, 0x00C4);   // Power Control 4
    Write_SPI_TFT_Reg(0xE8, 0x0040);   // Source OPON_N
    Write_SPI_TFT_Reg(0xE9, 0x0038);   // Source OPON_I
    Write_SPI_TFT_Reg(0xF1, 0x0001);
    Write_SPI_TFT_Reg(0xF2, 0x0010);
    Write_SPI_TFT_Reg(0x27, 0x00A3);   // Display Control 2

    // Power On sequence
    Write_SPI_TFT_Reg(0x1B, 0x001B);   // Power Control 2
    Write_SPI_TFT_Reg(0x1A, 0x0001);   // Power Control 1
    Write_SPI_TFT_Reg(0x24, 0x002F);   // Vcom Control 2
    Write_SPI_TFT_Reg(0x25, 0x0057);   // Vcom Control 3
    Write_SPI_TFT_Reg(0x23, 0x008D);   // Vcom Control 1

    // Gamma settings
    Write_SPI_TFT_Reg(0x40, 0x00);
    Write_SPI_TFT_Reg(0x41, 0x00);
    Write_SPI_TFT_Reg(0x42, 0x01);
    Write_SPI_TFT_Reg(0x43, 0x13);
    Write_SPI_TFT_Reg(0x44, 0x10);
    Write_SPI_TFT_Reg(0x45, 0x26);
    Write_SPI_TFT_Reg(0x46, 0x08);
    Write_SPI_TFT_Reg(0x47, 0x51);
    Write_SPI_TFT_Reg(0x48, 0x02);
    Write_SPI_TFT_Reg(0x49, 0x12);
    Write_SPI_TFT_Reg(0x4A, 0x18);
    Write_SPI_TFT_Reg(0x4B, 0x19);
    Write_SPI_TFT_Reg(0x4C, 0x14);
    Write_SPI_TFT_Reg(0x50, 0x19);
    Write_SPI_TFT_Reg(0x51, 0x2F);
    Write_SPI_TFT_Reg(0x52, 0x2C);
    Write_SPI_TFT_Reg(0x53, 0x3E);
    Write_SPI_TFT_Reg(0x54, 0x3F);
    Write_SPI_TFT_Reg(0x55, 0x3F);
    Write_SPI_TFT_Reg(0x56, 0x2E);
    Write_SPI_TFT_Reg(0x57, 0x77);
    Write_SPI_TFT_Reg(0x58, 0x0B);
    Write_SPI_TFT_Reg(0x59, 0x06);
    Write_SPI_TFT_Reg(0x5A, 0x07);
    Write_SPI_TFT_Reg(0x5B, 0x0D);
    Write_SPI_TFT_Reg(0x5C, 0x1D);
    Write_SPI_TFT_Reg(0x5D, 0xCC);

    // Power + Osc
    Write_SPI_TFT_Reg(0x18, 0x0036);   // OSC Control 1
    Write_SPI_TFT_Reg(0x19, 0x0001);   // OSC Control 2
    Write_SPI_TFT_Reg(0x01, 0x0000);   // Display Mode Control
    Write_SPI_TFT_Reg(0x1F, 0x0088);   // Power Control 6
    bcm2835_delay(5);
    Write_SPI_TFT_Reg(0x1F, 0x0080);   // Power Control 6
    bcm2835_delay(5);
    Write_SPI_TFT_Reg(0x1F, 0x0090);   // Power Control 6
    bcm2835_delay(5);
    Write_SPI_TFT_Reg(0x1F, 0x00D0);   // Power Control 6
    bcm2835_delay(5);

    // Appearance
    Write_SPI_TFT_Reg(0x17, 0x0005);   // Colmod 16Bit/Pixel
    Write_SPI_TFT_Reg(0x36, 0x0000);   // Panel Characteristic
    Write_SPI_TFT_Reg(0x28, 0x0038);   // Display Control 3
    bcm2835_delay(40);
    Write_SPI_TFT_Reg(0x28, 0x003C);   // Display Control 3
    bcm2835_delay(2);
    Write_SPI_TFT_Reg(0x16, 0x0008);        // Orientation
    Write_SPI_TFT_Reg(0x03, (Orig_X >> 0)); // Set Dimensions
    Write_SPI_TFT_Reg(0x02, (Orig_X >> 8));
    Write_SPI_TFT_Reg(0x05, ((Size_X - 1) >> 0));
    Write_SPI_TFT_Reg(0x04, ((Size_X - 1) >> 8));
    Write_SPI_TFT_Reg(0x07, (Orig_Y >> 0));
    Write_SPI_TFT_Reg(0x06, (Orig_Y >> 8));
    Write_SPI_TFT_Reg(0x09, ((Size_Y - 1) >> 0));
    Write_SPI_TFT_Reg(0x08, ((Size_Y - 1) >> 8));
}

void SPI_TFT_pixel(int x, int y, int color)
{
    // Set pixel window to a single point
    Write_SPI_TFT_Reg(0x03, (x >> 0));
    Write_SPI_TFT_Reg(0x02, (x >> 8));
    Write_SPI_TFT_Reg(0x05, (x >> 0));  // x2 = x1 (single pixel)
    Write_SPI_TFT_Reg(0x04, (x >> 8));
    Write_SPI_TFT_Reg(0x07, (y >> 0));
    Write_SPI_TFT_Reg(0x06, (y >> 8));
    Write_SPI_TFT_Reg(0x09, (y >> 0));  // y2 = y1 (single pixel)
    Write_SPI_TFT_Reg(0x08, (y >> 8));
    Write_SPI_TFT_Cmd(0x22);
    Write_SPI_TFT_Dat(color);
}

void SPI_TFT_region(int x1, int y1, int x2, int y2, int color)
{
    int num_pixels = (x2 - x1 + 1) * (y2 - y1 + 1);

    // Set the drawing window
    Write_SPI_TFT_Reg(0x03, (x1 >> 0));
    Write_SPI_TFT_Reg(0x02, (x1 >> 8));
    Write_SPI_TFT_Reg(0x05, (x2 >> 0));
    Write_SPI_TFT_Reg(0x04, (x2 >> 8));
    Write_SPI_TFT_Reg(0x07, (y1 >> 0));
    Write_SPI_TFT_Reg(0x06, (y1 >> 8));
    Write_SPI_TFT_Reg(0x09, (y2 >> 0));
    Write_SPI_TFT_Reg(0x08, (y2 >> 8));

    // Start pixel write command
    Write_SPI_TFT_Cmd(0x22);

    // Build a reusable 3-byte data packet and blast it for every pixel.
    // Using bcm2835_spi_transfern avoids the per-byte CS toggle overhead.
    uint8_t pkt[3];
    pkt[0] = 0x72;                          // Start byte: write + data
    pkt[1] = (uint8_t)(color >> 8);
    pkt[2] = (uint8_t)(color & 0xFF);

    for (int i = 0; i < num_pixels; i++)
    {
        bcm2835_spi_transfern((char *)pkt, 3);
    }
}