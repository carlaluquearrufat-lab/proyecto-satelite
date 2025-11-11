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
        String data = mySerial.readStringUntil('\n');
        Serial.print(data);
        procesarMensajeSatelite(data);

        if (data == "Fallo") {
            digitalWrite(pinAlarma, HIGH);
        } else {
            digitalWrite(pinAlarma, LOW);
            digitalWrite(5, HIGH);
            delay(500);
            digitalWrite(5, LOW);
            delay(500);
        }
    }
}

// FUERA DEL LOOP
void procesarMensajeSatelite(String mensaje) {
    int fin = mensaje.indexOf(':', 0);
    int codigo = mensaje.substring(0, fin).toInt();
    int inicio = fin + 1;

    switch (codigo) {
        case 1: { // Temperatura
            float temperatura = mensaje.substring(inicio).toFloat();
            Serial.println("Temperatura recibida: " + String(temperatura));
            break;
        }
        case 2: { // Humedad
            float humedad = mensaje.substring(inicio).toFloat();
            Serial.println("Humedad recibida: " + String(humedad));
            break;
        }
        case 3: { // Distancia
            int distancia = mensaje.substring(inicio).toInt();
            Serial.println("Distancia recibida: " + String(distancia));
            break;
        }
        case 4: { // Error de temperatura
            Serial.println("Error de captura de temperatura!");
            break;
        }
        case 5: { // Promedio temperatura
            float promTemp = mensaje.substring(inicio).toFloat();
            Serial.println("Promedio temperatura: " + String(promTemp));
            break;
        }
        case 6: { // Promedio humedad
            float promHum = mensaje.substring(inicio).toFloat();
            Serial.println("Promedio humedad: " + String(promHum));
            break;
        }
    }
}
