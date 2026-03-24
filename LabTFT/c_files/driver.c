
//FIB, DSBM, Enric X. Martin Rull, March 2016

#include "driver.h"

//TFT Interface Functions

void Config_Pins()
{
    //wiringPiSetup();
    wiringPiSetupGpio();
    pinMode(CS_Pin, OUTPUT);
    pinMode(Reset_Pin, OUTPUT);
    pinMode(SCLK_Pin, OUTPUT);
    pinMode(SDI_Pin, OUTPUT);
    pinMode(SDO_Pin, INPUT);

    // Initial values
    digitalWrite(CS_Pin, HIGH);
    digitalWrite(Reset_Pin, HIGH);
    digitalWrite(SCLK_Pin, LOW);
    delay(200);     //200 ms o 0.2s
}


/** NI IDEA DE COM PASSAR LA FUNCIÓ A C
 * def Free_Pins():
 *	GPIO.cleanup()
 *
 */

void CS_TFT(int value)
{
    digitalWrite(CS_Pin, value);
}

void Reset_TFT(int value)
{
    digitalWrite(Reset_Pin, value);
}

void Clock_SPI()
{
    digitalWrite(SCLK_Pin, LOW);
    digitalWrite(SCLK_Pin, HIGH);
}

void Send_SPI(int value)
{
    digitalWrite(SDI_Pin, value);
    Clock_SPI();
}

int Recv_SPI()
{
    Clock_SPI();
    return digitalRead(SDO_Pin);
}

void Send_SPI_8(int value)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        if (value & 0x80) Send_SPI(1);
        else Send_SPI(0);
        value = value << 1;
    }
}


//SPI Level Functions

#define SPI_START 0x70
#define SPI_RD 0x01
#define SPI_WR 0x00
#define SPI_DATA 0x02
#define SPI_INDEX 0x00

void Write_SPI_TFT_Cmd(int reg)
{
    //Reg is 8 bit
    CS_TFT(0);
    //Send Start,Write,Index
    Send_SPI_8(0x70);
    //Send the value
    Send_SPI_8(reg);
    CS_TFT(1);
}

void Write_SPI_TFT_Dat(int value)
{

    //value is 16 bit
    CS_TFT(0);
    //Send Start,Write,Data
    Send_SPI_8(0x72);
    //Send the value
    Send_SPI_8(value>>8);
    Send_SPI_8(value);
    CS_TFT(1);
}

void Write_SPI_TFT_Reg(int reg, int value)
{
    //Sets a value to a reg
    Write_SPI_TFT_Cmd(reg);
    Write_SPI_TFT_Dat(value);
}

int Read_SPI_TFT_Reg(int reg)
{
    //Sets a value to a reg
    Write_SPI_TFT_Cmd(reg);
    //Caldria llegir 16 bits
    return(0x0000);
}

//TFT Level Functions
void SPI_TFT_Reset()
{

    //Reset the TFT
    Reset_TFT(0);
    delay(250);
    Reset_TFT(1);
    delay(6);

    int driverCode = Read_SPI_TFT_Reg(0x00);

    //Start Initial Sequence
    Write_SPI_TFT_Reg(0xEA, 0x0000);    //Reset Power Control 1
    Write_SPI_TFT_Reg(0xEB, 0x0020);   //Power Control 2
    Write_SPI_TFT_Reg(0xEC, 0x000C);    //Power Control 3
    Write_SPI_TFT_Reg(0xED, 0x00C4);    //Power Control 4
    Write_SPI_TFT_Reg(0xE8, 0x0040);    //Source OPON_N
    Write_SPI_TFT_Reg(0xE9, 0x0038);    //Source OPON_I
    Write_SPI_TFT_Reg(0xF1, 0x0001);
    Write_SPI_TFT_Reg(0xF2, 0x0010);
    Write_SPI_TFT_Reg(0x27, 0x00A3);    //Display Control 2

    // Power On sequence
    Write_SPI_TFT_Reg(0x1B, 0x001B);    //Power Control 2
    Write_SPI_TFT_Reg(0x1A, 0x0001);    //Power Control 1
    Write_SPI_TFT_Reg(0x24, 0x002F);    //Vcom Control 2
    Write_SPI_TFT_Reg(0x25, 0x0057);    //Vcom Control 3
    Write_SPI_TFT_Reg(0x23, 0x008D);    //Vcom Control 1

    // Gamma settings
    Write_SPI_TFT_Reg(0x40,0x00);
    Write_SPI_TFT_Reg(0x41,0x00);
    Write_SPI_TFT_Reg(0x42,0x01);
    Write_SPI_TFT_Reg(0x43,0x13);
    Write_SPI_TFT_Reg(0x44,0x10);
    Write_SPI_TFT_Reg(0x45,0x26);
    Write_SPI_TFT_Reg(0x46,0x08);
    Write_SPI_TFT_Reg(0x47,0x51);
    Write_SPI_TFT_Reg(0x48,0x02);
    Write_SPI_TFT_Reg(0x49,0x12);
    Write_SPI_TFT_Reg(0x4A,0x18);
    Write_SPI_TFT_Reg(0x4B,0x19);
    Write_SPI_TFT_Reg(0x4C,0x14);
    Write_SPI_TFT_Reg(0x50,0x19);
    Write_SPI_TFT_Reg(0x51,0x2F);
    Write_SPI_TFT_Reg(0x52,0x2C);
    Write_SPI_TFT_Reg(0x53,0x3E);
    Write_SPI_TFT_Reg(0x54,0x3F);
    Write_SPI_TFT_Reg(0x55,0x3F);
    Write_SPI_TFT_Reg(0x56,0x2E);
    Write_SPI_TFT_Reg(0x57,0x77);
    Write_SPI_TFT_Reg(0x58,0x0B);
    Write_SPI_TFT_Reg(0x59,0x06);
    Write_SPI_TFT_Reg(0x5A,0x07);
    Write_SPI_TFT_Reg(0x5B,0x0D);
    Write_SPI_TFT_Reg(0x5C,0x1D);
    Write_SPI_TFT_Reg(0x5D,0xCC);

    //Power + Osc
    Write_SPI_TFT_Reg(0x18,0x0036);      //OSC Control 1
    Write_SPI_TFT_Reg(0x19,0x0001);      //OSC Control 2
    Write_SPI_TFT_Reg(0x01,0x0000);      //Display Mode Control
    Write_SPI_TFT_Reg(0x1F,0x0088);      //Power Control 6
    delay(5);
    Write_SPI_TFT_Reg(0x1F,0x0080);      //Power Control 6
    delay(5);
    Write_SPI_TFT_Reg(0x1F,0x0090);      //Power Control 6
    delay(5);
    Write_SPI_TFT_Reg(0x1F,0x00D0);      //Power Control 6
    delay(5);

    //Appearance
    Write_SPI_TFT_Reg(0x17,0x0005);      //Colmod 16Bit/Pixel
    Write_SPI_TFT_Reg(0x36,0x0000);      //Panel Characteristic
    Write_SPI_TFT_Reg(0x28,0x0038);      //Display Control 3
    delay(40);
    Write_SPI_TFT_Reg(0x28,0x003C);      //Display Control 3
    delay(2);
    Write_SPI_TFT_Reg(0x16,0x0008);	    //Orientation
    Write_SPI_TFT_Reg(0x03,(Orig_X>>0)); //Set Dimensions
    Write_SPI_TFT_Reg(0x02,(Orig_X>>8));
    Write_SPI_TFT_Reg(0x05,((Size_X-1)>>0));
    Write_SPI_TFT_Reg(0x04,((Size_X-1)>>8));
    Write_SPI_TFT_Reg(0x07,(Orig_Y>>0));
    Write_SPI_TFT_Reg(0x06,(Orig_Y>>8));
    Write_SPI_TFT_Reg(0x09,((Size_Y-1)>>0));
    Write_SPI_TFT_Reg(0x08,((Size_Y-1)>>8));
}

void SPI_TFT_pixel(int x, int y, int color)
{
    //Sets a Pixel X,Y to a Color
    Write_SPI_TFT_Reg(0x03,(x>>0));
    Write_SPI_TFT_Reg(0x02,(x>>8));
    Write_SPI_TFT_Reg(0x07,(y>>0));
    Write_SPI_TFT_Reg(0x06,(y>>8));
    Write_SPI_TFT_Cmd(0x22);
    Write_SPI_TFT_Dat(color);
}

void SPI_TFT_region(int x1, int y1, int x2, int y2, int color) {
    // Establecer las coordenadas de la región
    Write_SPI_TFT_Reg(0x03, (x1 >> 0));  // X1 (parte baja)
    Write_SPI_TFT_Reg(0x02, (x1 >> 8));  // X1 (parte alta)
    Write_SPI_TFT_Reg(0x07, (y1 >> 0));  // Y1 (parte baja)
    Write_SPI_TFT_Reg(0x06, (y1 >> 8));  // Y1 (parte alta)

    Write_SPI_TFT_Reg(0x50, (x2 >> 0));  // X2 (parte baja)
    Write_SPI_TFT_Reg(0x51, (x2 >> 8));  // X2 (parte alta)
    Write_SPI_TFT_Reg(0x52, (y2 >> 0));  // Y2 (parte baja)
    Write_SPI_TFT_Reg(0x53, (y2 >> 8));  // Y2 (parte alta)

    Write_SPI_TFT_Cmd(0x22);  // Comando para iniciar la escritura de píxeles

    // Iterar sobre la región para pintar cada píxel
    for (int x = x1; x <= x2; x++) {
      for (int y = y1; y <= y2; y++) {
            Write_SPI_TFT_Dat(color);  // Escribir el color para el píxel
        }
    }
}

