#include <SoftwareSerial.h>

SoftwareSerial LoRaSerial(10, 11); // RX, TX

const int pinAlarma = 8;
const int pinLed = 5;

unsigned long marcaAlarma = 0;
unsigned long marcaLed = 0;

bool alarmaActiva = false;
bool ledActivo = false;

void setup() {
    Serial.begin(9600);
    LoRaSerial.begin(9600);

    pinMode(pinLed, OUTPUT);
    pinMode(pinAlarma, OUTPUT);

    Serial.println("Sistema Tierra listo para recibir datos por LoRa...");
}

void loop() {
    // ----- Recepción de datos por LoRa -----
    if (LoRaSerial.available()) {
        String linea = LoRaSerial.readStringUntil('\n');
        linea.trim(); // eliminar \r y espacios

        if (linea.length() > 0) {
            // Mostrar línea completa en el monitor
            Serial.println(linea);

            // ----- Parseo de datos -----
            float temp = 0, hum = 0, dist = 0;
            int ang = 0, num = 0;

            // Extraer cada valor usando "tag:value"
            int idx;

            if ((idx = linea.indexOf("#:")) >= 0) 
                num = linea.substring(idx + 4, linea.indexOf(" ", idx)).toInt();
            
            if ((idx = linea.indexOf("1:")) >= 0) 
                temp = linea.substring(idx + 2, linea.indexOf(" ", idx)).toFloat();
            
            if ((idx = linea.indexOf("2:")) >= 0) 
                hum = linea.substring(idx + 2, linea.indexOf(" ", idx)).toFloat();
            
            if ((idx = linea.indexOf("3:")) >= 0) 
                dist = linea.substring(
                    idx + 5, 
                    linea.indexOf(" ", idx + 5) == -1 ? linea.length() : linea.indexOf(" ", idx + 5)
                ).toFloat();
            
            if ((idx = linea.indexOf("4:")) >= 0) 
                ang = linea.substring(idx + 4).toInt();

            // Mostrar datos parseados
            Serial.print("Lectura #: ");
            Serial.println(num);
            Serial.print("Temperatura: ");
            Serial.print(temp);
            Serial.println(" °C");
            Serial.print("Humedad: ");
            Serial.print(hum);
            Serial.println(" %");
            Serial.print("Distancia: ");
            Serial.print(dist);
            Serial.println(" cm");
            Serial.print("Ángulo servo: ");
            Serial.println(ang);
            Serial.println("--------------------------");

            // ----- Activar alarmas según contenido -----
            if (linea.indexOf("No Echo") >= 0) 
                activarAlarma(1000);
            else if (linea.indexOf("Error al leer") >= 0) 
                activarAlarma(2000);
            else 
                activarLed();
        }
    }

    // ----- Envío de comandos al satélite -----
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        LoRaSerial.println(cmd);
    }

    // ----- Efectos no bloqueantes -----
    actualizarAlarma();
    actualizarLed();
}

// ----- Funciones de alarma y LED -----
void activarAlarma(float freq) {
    tone(pinAlarma, freq);
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
