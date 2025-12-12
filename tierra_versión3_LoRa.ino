#include <SoftwareSerial.h>

SoftwareSerial LoRaSerial(10, 11); // RX, TX

const int pinAlarma = 8;
const int pinLed = 5;

// Constants
const double G = 6.67430e-11;  // Gravitational constant (m^3 kg^-1 s^-2)
const double M = 5.97219e24;   // Mass of Earth (kg)
const double R_EARTH = 6371000;  // Radius of Earth (meters)
const double ALTITUDE = 400000;  // Altitude of satellite above Earth's surface (meters)
const double EARTH_ROTATION_RATE = 7.2921159e-5;  // Earth's rotational rate (radians/second)
const unsigned long MILLIS_BETWEEN_UPDATES = 1000; // Time between orbit simulation updates
const double TIME_COMPRESSION = 90.0; // Time compression factor

unsigned long marcaAlarma = 0;
unsigned long marcaLed = 0;

bool alarmaActiva = false;
bool ledActivo = false;

// Variables
unsigned long nextUpdate;
double real_orbital_period;
double r;

// Forward declarations (prototipos)
void simulate_orbit(unsigned long millis, double inclination, int ecef);
void procesarComando(String cmd);
float medirDistancia();
void parpadeoLed(int ledPin, unsigned long &marca, unsigned long ahora);

void setup() {
    Serial.begin(9600);
    LoRaSerial.begin(9600);
        nextUpdate = MILLIS_BETWEEN_UPDATES;

    r = R_EARTH + ALTITUDE;
    real_orbital_period = 2 * PI * sqrt(pow(r, 3) / (G * M));

    pinMode(pinLed, OUTPUT);
    pinMode(pinAlarma, OUTPUT);

    Serial.println("Sistema Tierra listo para recibir datos por LoRa...");
}

void loop() {

  unsigned long currentTime = millis();
    if (currentTime > nextUpdate) {
        simulate_orbit(currentTime, 0, 0);
        nextUpdate = currentTime + MILLIS_BETWEEN_UPDATES;
    }
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
            if (linea.indexOf("1!2") >= 0) {
                activarAlarma(1000);
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

void simulate_orbit(unsigned long millis, double inclination, int ecef) {
    double time = (millis / 1000.0) * TIME_COMPRESSION;
    double angle = 2 * PI * (time / real_orbital_period);
    double x = r * cos(angle);
    double y = r * sin(angle) * cos(inclination);
    double z = r * sin(angle) * sin(inclination);

    if (ecef) {
        double theta = EARTH_ROTATION_RATE * time;
        double x_ecef = x * cos(theta) - y * sin(theta);
        double y_ecef = x * sin(theta) + y * cos(theta);
        x = x_ecef;
        y = y_ecef;
    }

    Serial.print("Time: ");
    Serial.print(time);
    Serial.print(" s | Position: (X: ");
    Serial.print(x);
    Serial.print(" m, Y: ");
    Serial.print(y);
    Serial.print(" m, Z: ");
    Serial.print(z);
    Serial.println(" m)");
}
