#!/usr/bin/env python

# TFT Graphics Library
# Basada en el driver SPI de Enric X. Martin Rull (FIB, DSBM, Marzo 2016)
# Libreria de funciones graficas de alto nivel para pantalla TFT 240x320

import RPi.GPIO as GPIO
import time as Time
import random
import driver


#  Conexiones: pin TFT = pin de placa (BOARD)
CS_Pin    = 24
Reset_Pin = 11
SCLK_Pin  = 23
SDO_Pin   = 21
SDI_Pin   = 19

#  Dimensiones de la pantalla
Orig_X = 0
Orig_Y = 0
Size_X = 240
Size_Y = 320

COLOR_WHITE  = 0xFFFF
COLOR_BLACK  = 0x0000
COLOR_RED    = 0xF800
COLOR_GREEN  = 0x07E0
COLOR_BLUE   = 0x001F
COLOR_YELLOW = 0xFFE0
COLOR_CYAN   = 0x07FF
COLOR_MAGENTA= 0xF81F

#  CAPA DE INTERFAZ GPIO / SPI  (driver base)
def Config_Pins():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(CS_Pin,    GPIO.OUT)
    GPIO.setup(Reset_Pin, GPIO.OUT)
    GPIO.setup(SCLK_Pin,  GPIO.OUT)
    GPIO.setup(SDI_Pin,   GPIO.OUT)
    GPIO.setup(SDO_Pin,   GPIO.IN)
    GPIO.output(CS_Pin,    True)
    GPIO.output(Reset_Pin, True)
    GPIO.output(SCLK_Pin,  False)
    Time.sleep(0.2)

def Free_Pins():
    GPIO.cleanup()

def CS_TFT(value):
    GPIO.output(CS_Pin, value)

def Reset_TFT(value):
    GPIO.output(Reset_Pin, value)

def Clock_SPI():
    GPIO.output(SCLK_Pin, False)
    GPIO.output(SCLK_Pin, True)

def Send_SPI(value):
    GPIO.output(SDI_Pin, value)
    Clock_SPI()

def Send_SPI_8(value):
    for _ in range(8):
        Send_SPI(bool(value & 128))
        value = value << 1

def Write_SPI_TFT_Cmd(reg):
    CS_TFT(False)
    Send_SPI_8(0x70)
    Send_SPI_8(reg)
    CS_TFT(True)

def Write_SPI_TFT_Dat(value):
    CS_TFT(False)
    Send_SPI_8(0x72)
    Send_SPI_8(value >> 8)
    Send_SPI_8(value)
    CS_TFT(True)

def Write_SPI_TFT_Reg(reg, value):
    Write_SPI_TFT_Cmd(reg)
    Write_SPI_TFT_Dat(value)

def Read_SPI_TFT_Reg(reg):
    Write_SPI_TFT_Cmd(reg)
    return 0x0000


#  INICIALIZACION DE LA PANTALLA
def SPI_TFT_Reset():
    Reset_TFT(False);  Time.sleep(0.25)
    Reset_TFT(True);   Time.sleep(0.006)

    Read_SPI_TFT_Reg(0x00)

    # Secuencia inicial
    Write_SPI_TFT_Reg(0xEA, 0x0000)
    Write_SPI_TFT_Reg(0xEB, 0x0020)
    Write_SPI_TFT_Reg(0xEC, 0x000C)
    Write_SPI_TFT_Reg(0xED, 0x00C4)
    Write_SPI_TFT_Reg(0xE8, 0x0040)
    Write_SPI_TFT_Reg(0xE9, 0x0038)
    Write_SPI_TFT_Reg(0xF1, 0x0001)
    Write_SPI_TFT_Reg(0xF2, 0x0010)
    Write_SPI_TFT_Reg(0x27, 0x00A3)

    # Power On
    Write_SPI_TFT_Reg(0x1B, 0x001B)
    Write_SPI_TFT_Reg(0x1A, 0x0001)
    Write_SPI_TFT_Reg(0x24, 0x002F)
    Write_SPI_TFT_Reg(0x25, 0x0057)
    Write_SPI_TFT_Reg(0x23, 0x008D)

    # Gamma
    gamma_vals = [0x00,0x00,0x01,0x13,0x10,0x26,0x08,0x51,
                  0x02,0x12,0x18,0x19,0x14,0x19,0x2F,0x2C,
                  0x3E,0x3F,0x3F,0x2E,0x77,0x0B,0x06,0x07,
                  0x0D,0x1D,0xCC]
    for i, v in enumerate(gamma_vals):
        Write_SPI_TFT_Reg(0x40 + i, v)

    # Oscilador y encendido
    Write_SPI_TFT_Reg(0x18, 0x0036)
    Write_SPI_TFT_Reg(0x19, 0x0001)
    Write_SPI_TFT_Reg(0x01, 0x0000)
    Write_SPI_TFT_Reg(0x1F, 0x0088); Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F, 0x0080); Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F, 0x0090); Time.sleep(0.005)
    Write_SPI_TFT_Reg(0x1F, 0x00D0); Time.sleep(0.005)

    # Apariencia
    Write_SPI_TFT_Reg(0x17, 0x0005)
    Write_SPI_TFT_Reg(0x36, 0x0000)
    Write_SPI_TFT_Reg(0x28, 0x0038); Time.sleep(0.04)
    Write_SPI_TFT_Reg(0x28, 0x003C); Time.sleep(0.002)
    Write_SPI_TFT_Reg(0x16, 0x0008)

    # Dimensiones
    Write_SPI_TFT_Reg(0x03, (Orig_X >> 0))
    Write_SPI_TFT_Reg(0x02, (Orig_X >> 8))
    Write_SPI_TFT_Reg(0x05, ((Size_X - 1) >> 0))
    Write_SPI_TFT_Reg(0x04, ((Size_X - 1) >> 8))
    Write_SPI_TFT_Reg(0x07, (Orig_Y >> 0))
    Write_SPI_TFT_Reg(0x06, (Orig_Y >> 8))
    Write_SPI_TFT_Reg(0x09, ((Size_Y - 1) >> 0))
    Write_SPI_TFT_Reg(0x08, ((Size_Y - 1) >> 8))


#  PRIMITIVAS GRAFICAS BASE
def draw_pixel(x, y, color):
    """Dibuja un pixel en (x, y) con el color RGB565 indicado."""
    #Write_SPI_TFT_Reg(0x03, (x >> 0))
    #Write_SPI_TFT_Reg(0x02, (x >> 8))
    #Write_SPI_TFT_Reg(0x07, (y >> 0))
    #Write_SPI_TFT_Reg(0x06, (y >> 8))
    #Write_SPI_TFT_Cmd(0x22)
    #Write_SPI_TFT_Dat(color)
    driver.SPI_TFT_pixel(x, y, color)


def draw_hline(x, y, length, color):
    """
    Dibuja una linea horizontal.

    Parametros:
        x      -- coordenada X de inicio
        y      -- coordenada Y (fija)
        length -- longitud en pixeles
        color  -- color RGB565
    """
    for i in range(length):
        draw_pixel(x + i, y, color)


def draw_vline(x, y, length, color):
    """
    Dibuja una linea vertical.

    Parametros:
        x      -- coordenada X (fija)
        y      -- coordenada Y de inicio
        length -- longitud en pixeles
        color  -- color RGB565
    """
    for i in range(length):
        draw_pixel(x, y + i, color)


# a
#  RECTANGULOS
# a

def draw_rectangle(x, y, width, height, color):
    """
    Dibuja el contorno de un rectangulo.

    Parametros:
        x, y         -- esquina superior izquierda
        width        -- ancho  en pixeles
        height       -- alto   en pixeles
        color        -- color RGB565 (usa COLOR_WHITE, COLOR_RED, etc.)

    Ejemplo:
        draw_rectangle(10, 10, 100, 60, COLOR_WHITE)
    """
    draw_hline(x,             y,              width,  color)  # lado superior
    draw_hline(x,             y + height - 1, width,  color)  # lado inferior
    draw_vline(x,             y,              height, color)  # lado izquierdo
    draw_vline(x + width - 1, y,              height, color)  # lado derecho


def draw_filled_rectangle(x, y, width, height, color):
    """
    Dibuja un rectangulo relleno.

    Parametros:
        x, y         -- esquina superior izquierda
        width        -- ancho  en pixeles
        height       -- alto   en pixeles
        color        -- color RGB565

    Ejemplo:
        draw_filled_rectangle(10, 10, 100, 60, COLOR_WHITE)
    """
    for row in range(height):
        draw_hline(x, y + row, width, color)


def draw_white_rectangle(x, y, width, height):
    """
    Dibuja el contorno de un rectangulo BLANCO.

    Parametros:
        x, y         -- esquina superior izquierda
        width        -- ancho  en pixeles
        height       -- alto   en pixeles

    Ejemplo:
        draw_white_rectangle(20, 20, 80, 50)
        -> rectangulo blanco de 80x50 px con esquina superior izq. en (20,20)
    """
    draw_rectangle(x, y, width, height, COLOR_WHITE)


def draw_white_filled_rectangle(x, y, width, height):
    """
    Dibuja un rectangulo BLANCO relleno.

    Parametros:
        x, y         -- esquina superior izquierda
        width        -- ancho  en pixeles
        height       -- alto   en pixeles

    Ejemplo:
        draw_white_filled_rectangle(20, 20, 80, 50)
    """
    draw_filled_rectangle(x, y, width, height, COLOR_WHITE)


#  UTILIDADES
def rgb_to_565(r, g, b):
    """
    Convierte un color RGB (0-255 cada canal) a formato RGB565.

    Parametros:
        r -- componente rojo   (0-255)
        g -- componente verde  (0-255)
        b -- componente azul   (0-255)

    Retorna:
        Entero de 16 bits en formato RGB565.

    Ejemplo:
        blanco = rgb_to_565(255, 255, 255)  ->  0xFFFF
        rojo   = rgb_to_565(255,   0,   0)  ->  0xF800
    """
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def random_color():
    """Genera un color RGB565 aleatorio."""
    return rgb_to_565(
        random.randint(0, 255),
        random.randint(0, 255),
        random.randint(0, 255)
    )


def clear_screen(color=COLOR_BLACK):
    """
    Limpia toda la pantalla con el color indicado (negro por defecto).

    Ejemplo:
        clear_screen()              # pantalla negra
        clear_screen(COLOR_WHITE)   # pantalla blanca
    """
    draw_filled_rectangle(0, 0, Size_X, Size_Y, color)


#  INICIALIZACION COMPLETA
def init():
    """
    Inicializa los pines GPIO y la pantalla TFT.
    Llama a esta funciOn una sola vez al inicio del programa.
    """
    Config_Pins()
    SPI_TFT_Reset()


def quit():
    """
    Libera los recursos GPIO.
    Llama a esta funciOn al cerrar el programa.
    """
    Free_Pins()
