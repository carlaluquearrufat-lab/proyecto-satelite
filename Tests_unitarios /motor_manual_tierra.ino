#include <SoftwareSerial.h>

// Comunicación con Arduino satélite
SoftwareSerial mySerial(10, 11); // RX, TX

void setup() {
  Serial.begin(9600);     
  mySerial.begin(9600);   

  Serial.println("Arduino tierra listo...");
}

void loop() {

  // Datos que vienen desde Python (PC)
  if (Serial.available()) {
    char c = Serial.read();
    mySerial.write(c);  // Reenvía al Arduino satélite
  }

  // Si quieres ver lo que responde el Arduino satélite
  if (mySerial.available()) {
    char c = mySerial.read();
    Serial.write(c);   // Lo manda de vuelta a la PC
  }
}
