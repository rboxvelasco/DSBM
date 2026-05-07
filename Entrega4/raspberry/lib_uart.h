#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#ifndef LIB_UART_H
#define LIB_UART_H


#define BAUD_RATE   B9600
#define SERIAL_PORT "/dev/ttyAMA0"

// Opens UART port
int open_serial(const char *port);


#endif // LIB_UART_H
