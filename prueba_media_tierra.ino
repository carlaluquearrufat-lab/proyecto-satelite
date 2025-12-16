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

// --- AQUI EL CAMBIO: Ya no es const, es una variable modificable ---
unsigned long millisBetweenUpdates = 1000; 

const double TIME_COMPRESSION = 90.0;

// --- Variables Control ---
unsigned long marcaAlarma = 0;
unsigned long marcaPantalla = 0;
unsigned long marcaLed1 = 0;
unsigned long marcaLed2 = 0;
bool alarmaActiva = false;
unsigned long INTERVALO_PANTALLA = 300;
unsigned long INTERVALO_LED = 500;

// --- NUEVA VARIABLE PARA CONTROLAR EL TIEMPO DEL MENSAJE DE CONEXIÓN ---
unsigned long momentoConexion = 0; // Guardará cuándo llegó el primer dato
bool conexionDetectada = false;    // Bandera para saber si ya conectamos

// --- NUEVAS VARIABLES PARA ALARMA DE USUARIO ---
float limiteMaxTemp = 100.0; // Valor por defecto alto
int contadorTempHigh = 0;    // Contador de veces seguidas

// -----------------------------------------------
// --- Variables Orbita ---
unsigned long nextUpdate;
double real_orbital_period;
double r;

//Constantes LCD
#define COLS 16
#define ROWS 2

// --- Declaración de funciones ---
void simulate_orbit(unsigned long millis_actual, double inclination, int ecef);
void activarAlarma(float freq, int duracion);
void actualizarAlarma();
void parpadeoLed();

//Textos pantalla
String texto1_fila1 = "Iniciamos";
String texto1_fila2 = "conexion";

String texto2_fila1 = "Conexion";
String texto2_fila2 = "establecida";

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);
  lcd.begin(COLS, ROWS);

  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);
  digitalWrite(pinLed, LOW);

  nextUpdate = millisBetweenUpdates; // Usamos la variable
  r = R_EARTH + ALTITUDE;
  real_orbital_period = 2 * PI * sqrt(pow(r, 3) / (G * M));

  Serial.println("Sistema Tierra listo.");
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(texto1_fila1); // Iniciamos
  lcd.setCursor(3, 1);
  lcd.print(texto1_fila2); // Conexion
  delay(2000);
}

void loop() {
  // ==========================================
  // 1. RECEPCIÓN DE DATOS (LoRa -> PC)
  // ==========================================
  if (LoRaSerial.available()) {
    String linea = LoRaSerial.readStringUntil('\n');
    linea.trim();

    if (linea.length() > 0) {
      
      parpadeoLed();
      
      // Si es la primera vez que recibimos datos, guardamos el tiempo
      if (!conexionDetectada) {
        conexionDetectada = true;
        momentoConexion = millis();
      }

      // --- Variables locales para parseo ---
      float temp = -999, hum = 0, dist = 0;
      int ang = 0, num = 0;
      int idx;
      float media = 0;

      // --- Parseo ---
      if ((idx = linea.indexOf("#:")) != -1) num = linea.substring(idx + 2).toInt();
      if ((idx = linea.indexOf("1:")) != -1) temp = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("2:")) != -1) hum = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("3:")) != -1) dist = linea.substring(idx + 2).toFloat();
      if ((idx = linea.indexOf("4:")) != -1) ang = linea.substring(idx + 2).toInt();
      if ((idx = linea.indexOf("5:")) != -1) media = linea.substring(idx + 2).toFloat();

      // --- CONTROL DE PANTALLA ---
      if (millis() - marcaPantalla >= INTERVALO_PANTALLA) {
        marcaPantalla = millis();
        lcd.clear();

        // LÓGICA DE LOS 2 SEGUNDOS:
        if (millis() - momentoConexion < 2000) {
           lcd.setCursor(3, 0);
           lcd.print(texto2_fila1); // Conexion
           lcd.setCursor(3, 1);
           lcd.print(texto2_fila2); // establecida
        } 
        else {
           lcd.setCursor(0, 0);
           lcd.print("Media Temp:");
           lcd.setCursor(0, 1);
           lcd.print(media);      // Mostramos la variable media recibida
           lcd.print(" C");
        }
      }

      // --- Enviar al PC ---
      Serial.print(" 1: "); Serial.print(temp);
      Serial.print(" 2: "); Serial.print(hum);
      Serial.print(" 3: "); Serial.print(dist);
      Serial.print(" 4: "); Serial.println(ang);
      Serial.print(" 5: "); Serial.println(media);

      // --- LÓGICA DE ALARMA DE USUARIO ---
      if (temp != -999) {
        if (temp > limiteMaxTemp) {
          contadorTempHigh++;
        } else {
          contadorTempHigh = 0;
        }

        if (contadorTempHigh >= 3) {
          activarAlarma(2000, 1000);
          Serial.println("ALERTA: LIMITE DE TEMPERATURA EXCEDIDO");
          contadorTempHigh = 0;
        }
      }

      // --- Alarma por error de sensores ---
      if (linea.indexOf("1!2") >= 0) {
        activarAlarma(1000, 300);
      }
    }
  }
  
  actualizarAlarma();

  // ==========================================
  // 2. COMANDOS DESDE PC
  // ==========================================
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    // CAMBIO ALARMA
    if (cmd.startsWith("MAXT:")) {
      String valorStr = cmd.substring(5);
      limiteMaxTemp = valorStr.toFloat();
    } 
    // NUEVO: CAMBIO FRECUENCIA ORBITA
    else if (cmd.startsWith("FREQ:")) {
      String valorStr = cmd.substring(5);
      long nuevaFreq = valorStr.toInt();
      if (nuevaFreq > 0) {
        millisBetweenUpdates = nuevaFreq; // Actualizamos la variable
        // Reiniciamos el timer para aplicar el cambio inmediatamente
        nextUpdate = millis() + millisBetweenUpdates; 
      }
    }
    // SI NO ES LOCAL, SE ENVIA AL SATELITE POR LORA
    else {
      LoRaSerial.println(cmd);
    }
  }

  // ==========================================
  // 3. SIMULACIÓN ORBITA
  // ==========================================
  unsigned long currentTime = millis();
  if (currentTime > nextUpdate) {
    simulate_orbit(currentTime, 0.5, 1);
    // Usamos la variable modificable aquí:
    nextUpdate = currentTime + millisBetweenUpdates;
  }
}

// ---------------- FUNCIONES AUXILIARES ----------------
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

void parpadeoLed() {
  digitalWrite(pinLed, HIGH);
  delay(10);
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
  Serial.print("POS ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(y);
  Serial.print(" ");
  Serial.println(z);
}
