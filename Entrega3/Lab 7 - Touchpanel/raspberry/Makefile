CC     = gcc
CFLAGS = -Wall -Wextra -O2
LIBS   = -lwiringPi -lm -lpthread
OBJS   = driver.o lib_tft.o lib_uart.o stb_image.o

# --- Programes principals ---
TARGETS = print_screen uart_basic uart_button uart_touchpad touchpad_ui
EXE_TARGETS = $(addsuffix .exe, $(TARGETS))

all: $(EXE_TARGETS)

# Regla generica per generar executables
$(EXE_TARGETS): %.exe: %.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compilacio d'objectes normals
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regla especial per stb_image
stb_image.o: stb_image.h
	$(CC) -DSTB_IMAGE_IMPLEMENTATION -x c -c $< -o $@

# Neteja
clean:
	rm -f $(addsuffix .o, $(TARGETS)) $(EXE_TARGETS)

realclean: clean
	rm -f $(OBJS)
