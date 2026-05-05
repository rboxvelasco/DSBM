#ifndef WIRINGPI_H
#define WIRINGPI_H

// setup
static inline int wiringPiSetup(void) { return 0; }
static inline int wiringPiSetupGpio(void) { return 0; }

// modes
#define INPUT  0
#define OUTPUT 1

// states
#define LOW  0
#define HIGH 1

// pins
static inline void pinMode(int pin, int mode) {
    (void)pin; (void)mode;
}

static inline void digitalWrite(int pin, int value) {
    (void)pin; (void)value;
}

static inline int digitalRead(int pin) {
    (void)pin;
    return 0;
}

static inline void delay(unsigned int ms) {
    (void)ms;
}

#endif