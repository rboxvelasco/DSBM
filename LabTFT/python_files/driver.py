#!/usr/bin/env python

#FIB, DSBM, Enric X. Martin Rull, March 2016

import RPi.GPIO as GPIO
import time as Time
import random


#Connections, TFT pin = board pin

CS_Pin    =  24 #Poseu els Pins vostres aqui
Reset_Pin =  11	#Poseu els Pins vostres aqui
SCLK_Pin  =  23	#Poseu els Pins vostres aqui
SDO_Pin   =  21	#Poseu els Pins vostres aqui
SDI_Pin   =  19	#Poseu els Pins vostres aqui


#TFT Interface Functions

def Config_Pins():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(CS_Pin,GPIO.OUT)
    GPIO.setup(Reset_Pin,GPIO.OUT)
    GPIO.setup(SCLK_Pin,GPIO.OUT)
    GPIO.setup(SDI_Pin,GPIO.OUT)
    GPIO.setup(SDO_Pin,GPIO.IN)
    #Initial values
    GPIO.output(CS_Pin,True)
    GPIO.output(Reset_Pin,True)
    GPIO.output(SCLK_Pin,False)
    Time.sleep(0.2)

def Free_Pins():
    GPIO.cleanup()

def CS_TFT(value):
    GPIO.output(CS_Pin,value)

def Reset_TFT(value):
    GPIO.output(Reset_Pin,value)

def Clock_SPI():
    GPIO.output(SCLK_Pin,False)
    GPIO.output(SCLK_Pin,True)

def Send_SPI(value):
    GPIO.output(SDI_Pin,value)
    Clock_SPI()

def Recv_SPI():
    Clock_SPI()
    return GPIO.Input(SDO_Pin)

def Send_SPI_8(value):
    for i in range(8):
        if(value&128):
            Send_SPI(True)
        else:
            Send_SPI(False)
        value=value<<1


#SPI Level Functions

# SPI_START 0x70
# SPI_RD 0x01
# SPI_WR 0x00
# SPI_DATA 0x02
# SPI_INDEX 0x00

def Write_SPI_TFT_Cmd(reg):
    #Reg is 8 bit
    CS_TFT(False)
    #Send Start,Write,Index
    Send_SPI_8(0x70)
    #Send the value
    Send_SPI_8(reg)
    CS_TFT(True)

def Write_SPI_TFT_Dat(value):
    #value is 16 bit
    CS_TFT(False)
    #Send Start,Write,Data
    Send_SPI_8(0x72)
    #Send the value
    Send_SPI_8(value>>8)
    Send_SPI_8(value)
    CS_TFT(True)

def Write_SPI_TFT_Reg(reg,value):
    #Sets a value to a reg
    Write_SPI_TFT_Cmd(reg)
    Write_SPI_TFT_Dat(value)

def Read_SPI_TFT_Reg(reg):
    #Sets a value to a reg
    Write_SPI_TFT_Cmd(reg)
    #Caldria llegir 16 bits
    return(0x0000)



#TFT Level Functions
Orig_X=0
Orig_Y=0
Size_X=240
Size_Y=320

def SPI_TFT_Reset():
    #Reset the TFT
    Reset_TFT(False)
    Time.sleep(0.25)
    Reset_TFT(True)
    Time.sleep(0.006)

    driverCode = Read_SPI_TFT_Reg(0x00)

    #Start Initial Sequence
    Write_SPI_TFT_Reg(0xEA, 0x0000)    #Reset Power Control 1
    Write_SPI_TFT_Reg(0xEB, 0x0020)    #Power Control 2
    Write_SPI_TFT_Reg(0xEC, 0x000C)    #Power Control 3
    Write_SPI_TFT_Reg(0xED, 0x00C4)    #Power Control 4
    Write_SPI_TFT_Reg(0xE8, 0x0040)    #Source OPON_N
    Write_SPI_TFT_Reg(0xE9, 0x0038)    #Source OPON_I
    Write_SPI_TFT_Reg(0xF1, 0x0001)
    Write_SPI_TFT_Reg(0xF2, 0x0010)
    Write_SPI_TFT_Reg(0x27, 0x00A3)    #Display Control 2

    # Power On sequence
    Write_SPI_TFT_Reg(0x1B, 0x001B)    #Power Control 2
    Write_SPI_TFT_Reg(0x1A, 0x0001)    #Power Control 1
    Write_SPI_TFT_Reg(0x24, 0x002F)    #Vcom Control 2
    Write_SPI_TFT_Reg(0x25, 0x0057)    #Vcom Control 3
    Write_SPI_TFT_Reg(0x23, 0x008D)    #Vcom Control 1

    # Gamma settings
    Write_SPI_TFT_Reg(0x40,0x00)
    Write_SPI_TFT_Reg(0x41,0x00)
    Write_SPI_TFT_Reg(0x42,0x01)
    Write_SPI_TFT_Reg(0x43,0x13)
    Write_SPI_TFT_Reg(0x44,0x10)
    Write_SPI_TFT_Reg(0x45,0x26)
    Write_SPI_TFT_Reg(0x46,0x08)
    Write_SPI_TFT_Reg(0x47,0x51)
    Write_SPI_TFT_Reg(0x48,0x02)
    Write_SPI_TFT_Reg(0x49,0x12)
    Write_SPI_TFT_Reg(0x4A,0x18)
    Write_SPI_TFT_Reg(0x4B,0x19)
    Write_SPI_TFT_Reg(0x4C,0x14)
    Write_SPI_TFT_Reg(0x50,0x19)
    Write_SPI_TFT_Reg(0x51,0x2F)
    Write_SPI_TFT_Reg(0x52,0x2C)
    Write_SPI_TFT_Reg(0x53,0x3E)
    Write_SPI_TFT_Reg(0x54,0x3F)
    Write_SPI_TFT_Reg(0x55,0x3F)
    Write_SPI_TFT_Reg(0x56,0x2E)
    Write_SPI_TFT_Reg(0x57,0x77)
    Write_SPI_TFT_Reg(0x58,0x0B)
    Write_SPI_TFT_Reg(0x59,0x06)
    Write_SPI_TFT_Reg(0x5A,0x07)
    Write_SPI_TFT_Reg(0x5B,0x0D)
    Write_SPI_TFT_Reg(0x5C,0x1D)
    Write_SPI_TFT_Reg(0x5D,0xCC)

    #Power + Osc
    Write_SPI_TFT_Reg(0x18,0x0036)      #OSC Control 1
    Write_SPI_TFT_Reg(0x19,0x0001)      #OSC Control 2
    Write_SPI_TFT_Reg(0x01,0x0000)      #Display Mode Control
    Write_SPI_TFT_Reg(0x1F,0x0088)      #Power Control 6
    Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F,0x0080)      #Power Control 6
    Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F,0x0090)      #Power Control 6
    Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F,0x00D0)      #Power Control 6
    Time.sleep(0.005)

    #Appearance
    Write_SPI_TFT_Reg(0x17,0x0005)      #Colmod 16Bit/Pixel
    Write_SPI_TFT_Reg(0x36,0x0000)      #Panel Characteristic
    Write_SPI_TFT_Reg(0x28,0x0038)      #Display Control 3
    Time.sleep(0.04)
    Write_SPI_TFT_Reg(0x28,0x003C)      #Display Control 3
    Time.sleep(0.002)
    Write_SPI_TFT_Reg(0x16,0x0008)	    #Orientation
    Write_SPI_TFT_Reg(0x03,(Orig_X>>0)) #Set Dimensions
    Write_SPI_TFT_Reg(0x02,(Orig_X>>8))
    Write_SPI_TFT_Reg(0x05,(Size_X-1>>0))
    Write_SPI_TFT_Reg(0x04,(Size_X-1>>8))
    Write_SPI_TFT_Reg(0x07,(Orig_Y>>0))
    Write_SPI_TFT_Reg(0x06,(Orig_Y>>8))
    Write_SPI_TFT_Reg(0x09,(Size_Y-1>>0))
    Write_SPI_TFT_Reg(0x08,(Size_Y-1>>8))


def SPI_TFT_pixel(x,y,color):
    #Sets a Pixel X,Y to a Color
    Write_SPI_TFT_Reg(0x03,(x>>0))
    Write_SPI_TFT_Reg(0x02,(x>>8))
    Write_SPI_TFT_Reg(0x07,(y>>0))
    Write_SPI_TFT_Reg(0x06,(y>>8))
    Write_SPI_TFT_Cmd(0x22)
    Write_SPI_TFT_Dat(color)



def random_color():
    r = random.randint(0, 31)
    g = random.randint(0, 63)
    b = random.randint(0, 31)
    return (r << 11) | (g << 5) | b

def random_pixel():
    x = random.randint(0, Size_X - 1)
    y = random.randint(0, Size_Y - 1)
    color = random_color()
    SPI_TFT_pixel(x, y, color)

