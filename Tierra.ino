#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX (azul, naranja)
unsigned long nextMillis = 500;
const int pinAlarma = 8;

void setup() {
   Serial.begin(9600);
   Serial.println("Empezamos la recepción");
   mySerial.begin(9600);
   pinMode(5, OUTPUT);
   pinMode(pinAlarma, OUTPUT);
}
void loop() {
   if (mySerial.available()) {
      String data = mySerial.readString();
      Serial.print(data);
         digitalWrite(5, HIGH);
         delay(500);
         digitalWrite (5, LOW);
         delay(500);
   }
   else {
      digitalWrite(pinAlarma, HIGH);
   }
}

