#include <Servo.h>
#include <SoftwareSerial.h>
#include <DHT.h>

// ------CONSTANTES -----
const double G = 6.67430e-11;  // Gravitational constant (m^3 kg^-1 s^-2)
const double M = 5.97219e24;   // Mass of Earth (kg)
const double R_EARTH = 6371000;  // Radius of Earth (meters)
const double ALTITUDE = 400000;  // Altitude of satellite above Earth's surface (meters)
const double EARTH_ROTATION_RATE = 7.2921159e-5;  // Earth's rotational rate (radians/second)
const unsigned long MILLIS_BETWEEN_UPDATES = 1000; // Time in milliseconds between each orbit simulation update
const double  TIME_COMPRESSION = 90.0; // Time compression factor (90x)

// ----- PINES -----
const int TRIG = 8;
const int ECO = 9;
const int LED = 3;
const int ledExito = 4;
const int ledError = 6;
const int BUZZER = 7;
const int DHTPIN = 2;
const int DHTTYPE = DHT11;

// ----- INTERVALOS -----
const unsigned long INTERVALO_SERVO = 20;
const unsigned long INTERVALO_DIST = 80;
const unsigned long INTERVALO_TEMP = 3000;
const unsigned long INTERVALO_HUM = 3000;
const unsigned long INTERVALO_ENVIO = 2000; // envío LoRa
const unsigned long INTERVALO_LED = 500;
const unsigned long nextUpdate;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoEnvio = 0;
unsigned long tiempoLedExito = 0;
unsigned long tiempoLedError = 0;
double real_orbital_period; //Periodo real acual del satelite
double r; //Duistancia entre el centro de la Tierra y el satelite

int anguloActual = 90;
int direccion = 1;
const int incremento = 1;

float DISTANCIA = 0;
float TEMPERATURA = 0;
float HUMEDAD = 0;

bool ISNANT = false;
bool ISNANH = false;

bool leertemperatura = true;
bool leerhumedad = true;
bool leerdistancia = true;

Servo servo;
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial LoRaSerial(10, 11); // RX, TX

int numeroEnvio = 1;

// ----- SETUP -----
void setup() {
  servo.attach(13);
  servo.write(anguloActual);

  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(ledExito, OUTPUT);
  pinMode(ledError, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Serial.begin(9600);
  LoRaSerial.begin(9600);

  dht.begin();
  Serial.println("SATÉLITE listo para enviar datos por LoRa...");
}

// ----- LOOP -----
void loop() {
  unsigned long ahora = millis();

  // -------------------
  // Lectura de comandos por LoRa
  // -------------------
  if (LoRaSerial.available()) {
    static String mensaje = "";
    char c = LoRaSerial.read();
    if (c == '\n') {
      mensaje.trim();
      procesarComando(mensaje);
      mensaje = "";
    } else mensaje += c;
  }

  // -------------------
  // Lectura de temperatura
  // -------------------
  if (leertemperatura && ahora - tiempoTemp >= INTERVALO_TEMP) {
    tiempoTemp = ahora;
    float t = dht.readTemperature();
    if (isnan(t)) {
      ISNANT = true;
      LoRaSerial.println("Error al leer temperatura");
    } else {
      TEMPERATURA = t;
      ISNANT = false;
    }
  }

  // -------------------
  // Lectura de humedad
  // -------------------
  if (leerhumedad && ahora - tiempoHum >= INTERVALO_HUM) {
    tiempoHum = ahora;
    float h = dht.readHumidity();
    if (isnan(h)) {
      ISNANH = true;
      LoRaSerial.println("Error al leer humedad");
    } else {
      HUMEDAD = h;
      ISNANH = false;
    }
  }

  // -------------------
  // LED de error y buzzer
  // -------------------
  if (ISNANT && ISNANH) {
    parpadeoLed(ledError, tiempoLedError, ahora);
    tone(BUZZER, 1000);
  } else {
    noTone(BUZZER);
  }

  // -------------------
  // Movimiento suave del servo
  // -------------------
  if (leerdistancia && ahora - tiempoServo >= INTERVALO_SERVO) {
    tiempoServo = ahora;
    anguloActual += incremento * direccion;
    if (anguloActual >= 180) direccion = -1;
    else if (anguloActual <= 0) direccion = 1;
    servo.write(anguloActual);
  }

  // -------------------
  // Medición de distancia ultrasónica
  // -------------------
  if (leerdistancia && ahora - tiempoDist >= INTERVALO_DIST) {
    tiempoDist = ahora;
    DISTANCIA = medirDistancia();
    if (DISTANCIA < 0) DISTANCIA = 0; // evitar -1
  }

  // -----------------
  // Simulación orbita
  // -----------------
  if(ahora>nextUpdate) {
    simulate_orbit(ahora, 0, 0);
    nextUpdate = ahora + MILLIS_BETWEEN_UPDATES;
  }
  
  // -------------------
  // Envío de datos por LoRa cada INTERVALO_ENVIO
  // -------------------
  if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
    tiempoEnvio = ahora;
    String mensaje = "Num:" + String(numeroEnvio++) +
                     " T:" + String(TEMPERATURA,1) +
                     " H:" + String(HUMEDAD,1) +
                     " Dist:" + String(DISTANCIA,1) +
                     " Ang:" + String(anguloActual);
    LoRaSerial.println(mensaje);

    // LED de envío exitoso
    parpadeoLed(ledExito, tiempoLedExito, ahora);
  }
}

// ----- FUNCIONES -----
float medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  unsigned long duracion = pulseIn(ECO, HIGH, 30000UL);
  if (duracion == 0) return -1.0;
  return duracion / 58.2;
}

void parpadeoLed(int ledPin, unsigned long &marca, unsigned long ahora) {
  if (ahora - marca >= 500) {
    marca = ahora;
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

void procesarComando(String cmd) {
  cmd.trim();
  if (cmd.indexOf("STOP") >= 0) leertemperatura = leerhumedad = leerdistancia = false;
  else if (cmd.indexOf("REANUDAR") >= 0) leertemperatura = leerhumedad = leerdistancia = true;
  else if (cmd.indexOf("PararT") >= 0) leertemperatura = false;
  else if (cmd.indexOf("PararH") >= 0) leerhumedad = false;
  else if (cmd.indexOf("PararD") >= 0) leerdistancia = false;
  else if (cmd.indexOf("IniciarT") >= 0) leertemperatura = true;
  else if (cmd.indexOf("IniciarH") >= 0) leerhumedad = true;
  else if (cmd.indexOf("IniciarD") >= 0) leerdistancia = true;
  else if (cmd.startsWith("DIR:")) {
    int ang = cmd.substring(4).toInt();
    if (ang < 0) ang = 0;
    if (ang > 180) ang = 180;
    servo.write(ang);
    anguloActual = ang;  
  }
}

void simulate_orbit(unsigned long millis, double inclination, int ecef) {
    double time = (millis / 1000) * TIME_COMPRESSION;  // Real orbital time
    double angle = 2 * PI * (time / real_orbital_period);  // Angle in radians
    double x = r * cos(angle);  // X-coordinate (meters)
    double y = r * sin(angle) * cos(inclination);  // Y-coordinate (meters)
    double z = r * sin(angle) * sin(inclination);  // Z-coordinate (meters)

    if (ecef) {
        double theta = EARTH_ROTATION_RATE * time;
        double x_ecef = x * cos(theta) - y * sin(theta);
        double y_ecef = x * sin(theta) + y * cos(theta);
        x = x_ecef;
        y = y_ecef;
    }
    // Enviar información por el serial port 
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
