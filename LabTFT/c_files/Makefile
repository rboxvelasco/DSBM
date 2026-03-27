CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS_CODE = -lwiringPi -lm
LIBS_UART = -lpthread -lwiringPi -lm

TARGET1 = code.exe
TARGET2 = uart_pic
TARGET3 = uart_copy

OBJS_CODE = code.o driver.o lib.o
OBJS_UART = uart.o driver.o lib.o
OBJS_UART_COPY = uart.o driver.o lib.o

all: $(TARGET1) $(TARGET2)

# --- Programa principal ---
$(TARGET1): $(OBJS_CODE)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_CODE)

# --- UART ---
$(TARGET2): $(OBJS_UART)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_UART)

# --- UART_COPY ---
$(TARGET): $(OBJS_UART_COPY)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_UART)

# --- Compilació d'objectes ---
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# --- Neteja ---
clean:
	rm -f *.o $(TARGET1) $(TARGET2)
