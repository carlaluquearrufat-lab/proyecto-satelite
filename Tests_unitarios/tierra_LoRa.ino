#include <SoftwareSerial.h>

// RX = 10, TX = 11
SoftwareSerial LoRaSerial(10, 11);

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);

  Serial.println("TIERRA lista. Esperando mensajes...");
  LoRaSerial.println("TEST");
}

void loop() {
  if (LoRaSerial.available()) {
    String msg = LoRaSerial.readStringUntil('\n');
    Serial.print("Recibido por LoRa: ");
    Serial.println(msg);
  }
}
