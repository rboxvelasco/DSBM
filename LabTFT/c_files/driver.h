
#ifndef DRIVER_H
#define DRIVER_H

#include<wiringPi.h>
#include<stdio.h>
#include "ascii5x7.h"

//Connections, TFT pin = board pin
#define Reset_Pin   17
#define CS_Pin      8
#define SCLK_Pin    11
#define SDO_Pin     9
#define SDI_Pin     10

//Screen positions
#define Orig_X      0
#define Orig_Y      0
#define Size_X      240
#define Size_Y      320

//SPI constants
#define SPI_START   0x70
#define SPI_RD      0x01
#define SPI_WR      0x00
#define SPI_DATA    0x02
#define SPI_INDEX   0x00


//Low level functions
void Config_Pins(void);
void CS_TFT(int value);
void Reset_TFT(int value);
void Clock_SPI(void);
void Send_SPI(int value);
int  Recv_SPI(void);
void Send_SPI_8(int value);

//SPI level
void Write_SPI_TFT_Cmd(int reg);
void Write_SPI_TFT_Dat(int value);
void Write_SPI_TFT_Reg(int reg, int value);
int  Read_SPI_TFT_Reg(int reg);

//TFT level
void SPI_TFT_Reset(void);
void SPI_TFT_pixel(int x, int y, int color);
void SPI_TFT_region(int x1, int y1, int x2, int y2, int color);

#endif

/*
#ifndef DRIVER_H
#define DRIVER_H

#include <bcm2835.h>
#include <stdio.h>
#include "ascii5x7.h"

// Pins en numeració BCM (igual que abans)
#define Reset_Pin   17
#define CS_Pin       8   // NO usar com a CE hardware; control manual
#define SCLK_Pin    11
#define SDO_Pin      9
#define SDI_Pin     10

#define Orig_X   0
#define Orig_Y   0
#define Size_X   240
#define Size_Y   320

#define SPI_START  0x70
#define SPI_DATA   0x02

void Config_Pins(void);
void CS_TFT(int value);
void Reset_TFT(int value);
void Send_SPI_8(uint8_t value);
void Write_SPI_TFT_Cmd(int reg);
void Write_SPI_TFT_Dat(int value);
void Write_SPI_TFT_Reg(int reg, int value);
int  Read_SPI_TFT_Reg(int reg);
void SPI_TFT_Reset(void);
void SPI_TFT_pixel(int x, int y, int color);

// NOU: dibuix de rectangle optimitzat
void SPI_TFT_region(int x, int y, int w, int h, int color);

#endif
*/
