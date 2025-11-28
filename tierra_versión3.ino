#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
const int pinAlarma = 8;
const int pinLed = 5;

// Estados para parpadeo sin bloquear
unsigned long marcaAlarma = 0;
unsigned long marcaLed = 0;
bool alarmaActiva = false;
bool ledActivo = false;

String linea = "";

void setup() {
  Serial.begin(9600);
  Serial.println("Empezamos la recepción");
  mySerial.begin(9600);

  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);
}

void loop() {
  // LECTURA DESDE EL SATÉLITE
  while (mySerial.available() > 0) {
    char texto = mySerial.read();

    if (texto == '\n' || texto == '\r') {
      linea.trim();
      if (linea.length() > 0) {
        Serial.println(linea);

        if (linea.indexOf("No Echo") >= 0) {
          activarAlarma();
        } else {
          activarLed();
        }
      }
      linea = "";
    } 
    else {
      linea += texto;
    }
  }

  // ENVÍO DE INSTRUCCIONES A SATÉLITE
  if (Serial.available()) {
    char instrucciones = Serial.read();
    mySerial.println(instrucciones);
  }

  // EFECTOS NO BLOQUEANTES
  actualizarAlarma();
  actualizarLed();
}


// EFECTOS NO BLOQUEANTES

void activarAlarma() {
  tone(pinAlarma, 1000);
  alarmaActiva = true;
  marcaAlarma = millis();
}

void actualizarAlarma() {
  if (alarmaActiva && millis() - marcaAlarma >= 300) {
    noTone(pinAlarma);
    alarmaActiva = false;
  }
}

void activarLed() {
  digitalWrite(pinLed, HIGH);
  ledActivo = true;
  marcaLed = millis();
}

void actualizarLed() {
  if (ledActivo && millis() - marcaLed >= 150) {
    digitalWrite(pinLed, LOW);
    ledActivo = false;
  }
}
