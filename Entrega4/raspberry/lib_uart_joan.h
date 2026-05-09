#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <wiringPi.h>

#ifndef LIB_UART_H
#define LIB_UART_H

#define BAUD_RATE   B9600
#define SERIAL_PORT "/dev/ttyAMA0"

#define PIN_SEND_PERMISSION 13  // GPIO13 (BCM)
#define PACKET_SIZE 7
#define SYNC_BYTE   0xAA

// Opens UART port
int open_serial(const char *port);
int start_UART();
int read_message(uint16_t *x, uint16_t *y, int8_t *light);
void config_Pin();
void set_Pin(int value);
int close_UART();

#endif // LIB_UART_H
