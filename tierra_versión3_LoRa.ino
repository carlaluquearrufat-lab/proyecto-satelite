#include <SoftwareSerial.h>

SoftwareSerial LoRaSerial(10, 11); // RX, TX

const int pinAlarma = 8;
const int pinLed = 5;

// --- Constantes Orbita ---
const double G = 6.67430e-11;   // Gravitational constant
const double M = 5.97219e24;    // Mass of Earth
const double R_EARTH = 6371000; // Radius of Earth
const double ALTITUDE = 400000; // Altitude of satellite
const double EARTH_ROTATION_RATE = 7.2921159e-5;
const unsigned long MILLIS_BETWEEN_UPDATES = 1000; // Actualizar cada 1 seg
const double TIME_COMPRESSION = 90.0; // Factor de velocidad

// --- Variables Control ---
unsigned long marcaAlarma = 0;
unsigned long marcaLed = 0;
bool alarmaActiva = false;

// --- Variables Orbita ---
unsigned long nextUpdate;
double real_orbital_period;
double r;

// --- Declaración de funciones ---
void simulate_orbit(unsigned long millis_actual, double inclination, int ecef);
void activarAlarma(float freq);
void actualizarAlarma();
void parpadeoLed();

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);
  
  // Configuración pines
  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);
  digitalWrite(pinLed, LOW);

  // Inicializar cálculos órbita
  nextUpdate = MILLIS_BETWEEN_UPDATES;
  r = R_EARTH + ALTITUDE;
  real_orbital_period = 2 * PI * sqrt(pow(r, 3) / (G * M));

  Serial.println("Sistema Tierra listo.");
}

void loop() {

  // ==========================================
  // 1. RECEPCIÓN DE DATOS (LoRa -> PC)
  // ==========================================
  if (LoRaSerial.available()) {
    String linea = LoRaSerial.readStringUntil('\n');
    linea.trim(); // Limpiar espacios y saltos

    if (linea.length() > 0) {
      // Activar LED brevemente al recibir datos
      parpadeoLed();

      // Variables para guardar datos
      float temp = 0, hum = 0, dist = 0;
      int ang = 0, num = 0;
      int idx;

      // --- Parseo Seguro ---
      // Buscamos las etiquetas y extraemos el valor hasta el siguiente espacio
      if ((idx = linea.indexOf("#:")) != -1) num = linea.substring(idx + 2).toInt();
      if ((idx = linea.indexOf("1:")) != -1) temp = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("2:")) != -1) hum = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("3:")) != -1) dist = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("4:")) != -1) ang = linea.substring(idx + 2).toInt();

      // --- Enviar al PC (Monitor Serial / Python) ---
      // Python leerá estas lineas para las gráficas
      Serial.print("1: "); Serial.print(temp); 
      Serial.print(" 2: "); Serial.print(hum);
      Serial.print(" 3: "); Serial.print(dist);
      Serial.print(" 4: "); Serial.println(ang);

      // --- Lógica de Alarmas ---
      if (linea.indexOf("1!2") >= 0) {
        activarAlarma(1000); // Tono de 1000Hz
      }
    }
  }

  // Actualizar estado del buzzer (si está sonando)
  actualizarAlarma();


  // ==========================================
  // 2. COMANDOS DESDE PC (PC -> LoRa)
  // ==========================================
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    LoRaSerial.println(cmd); // Reenviar al satélite
  }


  // ==========================================
  // 3. SIMULACIÓN ORBITA
  // ==========================================
  unsigned long currentTime = millis();
  if (currentTime > nextUpdate) {
    simulate_orbit(currentTime, 0.5, 1); // Inclination 0.5 rad, ECEF activado
    nextUpdate = currentTime + MILLIS_BETWEEN_UPDATES;
  }
}

// ---------------- FUNCIONES AUXILIARES ----------------

void activarAlarma(float freq) {
  tone(pinAlarma, freq);
  alarmaActiva = true;
  marcaAlarma = millis();
}

void actualizarAlarma() {
  // La alarma suena solo por 300ms
  if (alarmaActiva && millis() - marcaAlarma >= 300) {
    noTone(pinAlarma);
    alarmaActiva = false;
  }
}

void parpadeoLed() {
  digitalWrite(pinLed, HIGH);
  delay(10); // Pequeño delay para que se vea el parpadeo
  digitalWrite(pinLed, LOW);
}

void simulate_orbit(unsigned long millis_val, double inclination, int ecef) {
  double time = (millis_val / 1000.0) * TIME_COMPRESSION;
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

  // IMPORTANTE: El formato debe empezar por "POS" para que Python lo lea
  Serial.print("POS ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.println(z);
}
