#include <Arduino.h>
#include <math.h>

const int PinIn = 7;   // Digital 7 = Entrada
const int PinOut = 8;  // Digital 8 = Sortida
const float C = 1; // Capacitat (en uF) del condensador

void setup() {
  Serial.begin(9600);
  pinMode(PinOut, OUTPUT);
  pinMode(PinIn, INPUT);
}

void discharge_cap() {
  // Podem tot com a output digital = 0
  digitalWrite(PinOut, LOW);
  pinMode(PinIn, OUTPUT);
  digitalWrite(PinIn, LOW);
  
  delay(1000); // Esperem 1 segon perquè es descarregui el condensador
  
  pinMode(PinIn, INPUT); // Restaurem pin d'entrada
}

unsigned long charge_time() {
  unsigned long t0, t1, elapsed;
  
  digitalWrite(PinOut, HIGH); // Comencem a carregar
  t0 = micros();

  while (digitalRead(PinIn) == LOW) {} // Esperem fins que arribi a HIGH (V_h min)
  
  t1 = micros();
  elapsed = t1 - t0;
  
  return elapsed;
}

void loop() {
  discharge_cap();
  unsigned long t = charge_time();

  Serial.print("Temps de càrrega: ");
  Serial.print(t);
  Serial.println(" us");
  
  double r = - ((t) / (C * log(1 - 2.5/5)));
  Serial.print("Resistència calculada: ");
  Serial.println(r);
  Serial.println();

  double integral = (t / C + r);
  double passada = integral ^ 
}
