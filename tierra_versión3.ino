#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
const int pinAlarma = 8;
const int pinLed = 5;

void setup() {
  Serial.begin(9600);
  Serial.println("Empezamos la recepción");
  mySerial.begin(9600);
  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);
}

String linea = "";

void loop() {

  while (mySerial.available()>0) {
    char texto = mySerial.read();

    // cuando llega una línea completa
    if (texto == '\n') {
      linea.trim();
      if (linea.length() > 0) {
        Serial.println(linea);

        if (linea.indexOf("No Echo") >= 0) {
          tone(pinAlarma, 1000);
          delay(300);
          noTone(pinAlarma);
        } else {
          digitalWrite(pinLed, HIGH);
          delay(150);
          digitalWrite(pinLed, LOW);
        }
      }
      linea = ""; // limpiar para la siguiente línea
    } 
    else {
      linea += texto; // acumular caracteres
    }

    //enviar a satelite instrucciones recibidas de la interfaz (parar,reanudar...)
    if (Serial.available()){
      char instrucciones= Serial.read();
      mySerial.println(instrucciones);
    }
    
  }
  //enviar a satelite instrucciones recibidas de la interfaz (parar,reanudar...) aunque no haya recibido ninguna información del satelite
  if (Serial.available()){
    char instrucciones= Serial.read();
    mySerial.println(instrucciones);
  }
}

