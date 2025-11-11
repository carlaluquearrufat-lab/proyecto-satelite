#include <SoftwareSerial.h>
int LED= 5;
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);// Comunicación con satélite
  Serial.println("Estacion de tierra lista");
  Serial.println ("Ingrese un angulo (0-180) y presione enviar:");
}

void loop() {
  // Leer datos desde la estación
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // leer línea completa
    input.trim();

  if (input.length() > 0) {
      int angulo = input.toInt();
      if (angulo >= 0 && angulo <= 180) {
        // Enviar al satélite
        Serial.println(angulo);
        mySerial.print("Enviado al satelite: ");
        mySerial.println(angulo);
      } else {
        Serial.println("Angulo invalido, ingrese un valor entre 0 y 180");
      }
    }
  }

  // Recibir datos del satélite ---
  while (mySerial.available() > 0) {
    String datos = Serial.readStringUntil('\n');
    datos.trim();
    if (datos.length() > 0) {
      Serial.print("Datos recibidos del satelite: ");
      Serial.println(datos);
      if (datos == 'Error en los datos de distancia'){
        digitalWrite (LED, HIGH);
        delay (500);
        digitalWrite (LED, LOW);
        delay (500);
      }
    }  
  }
}
