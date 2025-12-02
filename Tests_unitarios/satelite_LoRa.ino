#include <SoftwareSerial.h>

SoftwareSerial LoRaSerial(10, 11); // RX, TX

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);

  Serial.println("SATÉLITE listo. Enviando mensajes...");
}

void loop() {
  // Enviar mensaje de prueba
  LoRaSerial.println("Hola desde el SATÉLITE");
  Serial.println("Mensaje enviado");

  delay(2000); // cada 2 segundos
}
