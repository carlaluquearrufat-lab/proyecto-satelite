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

   if(serial.available()){
    String data= Serial.readSteingUntil('\n");
    data.trim();
   if (command.length()>0){
    mySerial.println(command);}}
  
  while (mySerial.available()) {
    char texto = mySerial.read();


    // cuando llega una línea completa
    if (texto == '\n') {
      linea.trim();
      if (linea.length() > 0) {
        Serial.println(linea);

        if (linea.indexOf("FalloDHT") >= 0 || linea.indexOf("NoEcho") >= 0) {
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
  }

  delay(5);
}
