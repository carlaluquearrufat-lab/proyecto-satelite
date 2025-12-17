#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

SoftwareSerial LoRaSerial(10, 11); // RX, TX
LiquidCrystal lcd(13, 12, 6, 4, 3, 2);

const int pinAlarma = 7;
const int pinLed = 5;

// --- Constantes Orbita ---
const double G = 6.67430e-11;   
const double M = 5.97219e24;    
const double R_EARTH = 6371000; 
const double ALTITUDE = 400000; 
const double EARTH_ROTATION_RATE = 7.2921159e-5;
unsigned long millisBetweenUpdates = 1000; // ahora variable modificable
const double TIME_COMPRESSION = 90.0;

// --- Variables Control ---
unsigned long marcaAlarma = 0;
bool alarmaActiva = false;

// --- Variables Orbita ---
unsigned long nextUpdate;
double real_orbital_period;
double r;

// --- Variables LCD / Media ---

float limiteMaxTemp = 30.0; // temperatura máxima configurable
unsigned long momentoConexion = 0;
// Control conexión
bool conexionDetectada = false;

// Control LCD
unsigned long marcaPantalla = 0;
const unsigned long INTERVALO_PANTALLA = 800; // más estable

// Alarma usuario robusta
int contadorTempHigh = 0;

// Variable para la media recibida del satélite
float mediaRecibida = 0;

// --- Funciones ---
void simulate_orbit(unsigned long millis_actual, double inclination, int ecef);
void activarAlarma(float freq, int duracion);
void actualizarAlarma();
void parpadeoLed(unsigned long &marca, int pin);

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);
  
  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);
  digitalWrite(pinLed, LOW);

  r = R_EARTH + ALTITUDE;
  real_orbital_period = 2 * PI * sqrt(pow(r, 3) / (G * M));

  nextUpdate = millis() + millisBetweenUpdates;

  lcd.begin(16, 2);
  lcd.print("Sistema listo");

  Serial.println("Sistema Tierra listo.");
}

void loop() {
  unsigned long ahora = millis();

  // ==========================================
  // 1. RECEPCIÓN DE DATOS (LoRa -> PC)
  // ==========================================
  while (LoRaSerial.available()) {
    String linea = LoRaSerial.readStringUntil('\n');
    linea.trim(); 
    if (linea == "OK") {
      Serial.println("OK");
      continue; 
    }

    if (!conexionDetectada) {
      conexionDetectada = true;
      momentoConexion = ahora;
    }

    if (linea.length() > 0) {
      parpadeoLed(marcaPantalla, pinLed);

      float temp = 0, hum = 0, dist = 0;
      int ang = 0, num = 0;
      int idx;

      // --- Parseo seguro ---
      if ((idx = linea.indexOf("#:")) != -1) num = linea.substring(idx + 2).toInt();
      if ((idx = linea.indexOf("1:")) != -1) temp = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("2:")) != -1) hum = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("3:")) != -1) dist = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("4:")) != -1) ang = linea.substring(idx + 2).toInt();
      if ((idx = linea.indexOf("5:")) != -1) mediaRecibida = linea.substring(idx + 2).toFloat();

      // --- Media de temperatura ---
      if (!isnan(mediaRecibida)) {
        if (mediaRecibida > limiteMaxTemp) {
            contadorTempHigh++;
        }   else {
              contadorTempHigh = 0;
        }

        if (contadorTempHigh >= 3) {
            activarAlarma(2000, 1000);
            Serial.println("ALERTA: LIMITE DE TEMPERATURA EXCEDIDO (MEDIA)");
            contadorTempHigh = 0;
        }
      }

      // --- Enviar al PC ---
      Serial.print("1: "); Serial.print(temp); 
      Serial.print(" 2: "); Serial.print(hum);
      Serial.print(" 3: "); Serial.print(dist);
      Serial.print(" 4: "); Serial.println(ang);
      Serial.print(" 5: "); Serial.println(mediaRecibida);


      // --- Lógica de Alarmas ---
      if (linea.indexOf("1!2") >= 0) {
        activarAlarma(1000, 300); 
      }

      // --- LCD ---
      if (ahora - marcaPantalla >= INTERVALO_PANTALLA) {
          marcaPantalla = ahora;
          lcd.clear();
          
          if (ahora - momentoConexion < 2000) {
              lcd.setCursor(3, 0);
              lcd.print("Conexion");
              lcd.setCursor(3, 1);
              lcd.print("establecida");
          } else {
              lcd.setCursor(0, 0);
              lcd.print("Media Temp:");
              lcd.setCursor(0, 1);
              lcd.print(mediaRecibida, 1);
              lcd.print(" C");
          }
      }
    }
  }

  // ==========================================
  // 2. COMANDOS DESDE PC (PC -> LoRa)
  // ==========================================
  while (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("MAXT:")) {
      limiteMaxTemp = cmd.substring(5).toFloat();
      Serial.print("Nuevo limite temp: ");
      Serial.println(limiteMaxTemp);
    }
    else if (cmd.startsWith("FREQ:")) {
      long nuevaFreq = cmd.substring(5).toInt();
      if (nuevaFreq > 0) {
        millisBetweenUpdates = nuevaFreq;
        nextUpdate = ahora + millisBetweenUpdates;
      }
    }
    else {
      LoRaSerial.println(cmd);
    }
  }

  // ==========================================
  // 3. SIMULACIÓN ORBITA
  // ==========================================
  if (ahora > nextUpdate) {
    simulate_orbit(ahora, 0.5, 1);
    nextUpdate = ahora + millisBetweenUpdates;
  }

  // Actualizar estado del buzzer
  actualizarAlarma();
}

// ---------------- FUNCIONES ----------------

unsigned long duracionAlarma = 300;

void activarAlarma(float freq, int duracion) {
    tone(pinAlarma, freq);
    alarmaActiva = true;
    marcaAlarma = millis();
    duracionAlarma = duracion;
}

void actualizarAlarma() {
  if (alarmaActiva && millis() - marcaAlarma >= duracionAlarma) {
    noTone(pinAlarma);
    alarmaActiva = false;
  }
}

void parpadeoLed(unsigned long &marca, int pin) {
  if (millis() - marca >= 100) {
    marca = millis();
    digitalWrite(pin, !digitalRead(pin));
  }
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

  Serial.print("POS ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.println(z);
}
