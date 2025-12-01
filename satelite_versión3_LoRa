#include <Servo.h>
#include <LoRa.h>
#include <DHT.h>

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
const unsigned long INTERVALO_LED = 500;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoLedExito = 0;
unsigned long tiempoLedError = 0;

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

int contadorErrores = 0;
int numeroEnvio = 1;

// ---------- SETUP ----------
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

  dht.begin();

  // ------------ INICIO DE LoRa ------------
  if (!LoRa.begin(433E6)) {        // Usa la frecuencia correspondiente
    Serial.println("Error al iniciar LoRa");
    while (1);
  }
  Serial.println("LoRa iniciado correctamente");
}

// ---------- LOOP ----------
void loop() {
  unsigned long ahora = millis();

  // -------------------
  // Envío de número de lectura
  // -------------------
  LoRa.beginPacket();
  LoRa.println(numeroEnvio++);
  LoRa.endPacket();

  // -------------------
  // Lectura de comandos por LoRa
  // -------------------
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    static String mensaje = "";
    while (LoRa.available()) {
      char c = LoRa.read();
      if (c == '\n') {
        mensaje.trim();
        procesarComando(mensaje);
        mensaje = "";
      } else {
        mensaje += c;
      }
    }
  }

  // -------------------
  // Lectura de temperatura
  // -------------------
  if (leertemperatura && ahora - tiempoTemp >= INTERVALO_TEMP) {
    tiempoTemp = ahora;
    float t = dht.readTemperature();
    if (isnan(t)) {
      ISNANT = true;
      contadorErrores++;

      LoRa.beginPacket();
      LoRa.println("Error al leer temperatura");
      LoRa.endPacket();

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
      contadorErrores++;

      LoRa.beginPacket();
      LoRa.println("Error al leer humedad");
      LoRa.endPacket();

    } else {
      HUMEDAD = h;
      ISNANH = false;
    }
  }

  // -------------------
  // LED de error si falla temp y humedad
  // -------------------
  if (ISNANT && ISNANH) {
    parpadeoLed(ledError, tiempoLedError, ahora);
    tone(BUZZER, 1000);
  } else {
    noTone(BUZZER);
  }

  // -------------------
  // Movimiento del servo
  // -------------------
  if (leerdistancia && ahora - tiempoServo >= INTERVALO_SERVO) {
    tiempoServo = ahora;
    anguloActual += incremento * direccion;
    if (anguloActual >= 180) direccion = -1;
    else if (anguloActual <= 0) direccion = 1;
    servo.write(anguloActual);
  }

  // -------------------
  // Medición de distancia
  // -------------------
  if (leerdistancia && ahora - tiempoDist >= INTERVALO_DIST) {
    tiempoDist = ahora;
    DISTANCIA = medirDistancia();
    if (DISTANCIA == -1.0) DISTANCIA = 0;

    LoRa.beginPacket();
    LoRa.print("DATA ");
    LoRa.print(anguloActual);
    LoRa.print(",");
    LoRa.println(DISTANCIA, 1);
    LoRa.endPacket();
  }

  // -------------------
  // LED de envío exitoso
  // -------------------
  parpadeoLed(ledExito, tiempoLedExito, ahora);

  // -------------------
  // Envío de todos los datos
  // -------------------
  LoRa.beginPacket();
  LoRa.print("T: "); LoRa.print(TEMPERATURA, 1);
  LoRa.print(" H: "); LoRa.print(HUMEDAD, 1);
  LoRa.print(" Dist(cm): ");
  if (DISTANCIA < 0) LoRa.println("No Echo");
  else LoRa.println(DISTANCIA, 1);
  LoRa.endPacket();
}

// ---------- FUNCIONES ----------
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
  if (ahora - marca >= INTERVALO_LED) {
    marca = ahora;
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

void procesarComando(String cmd) {
  if (cmd.indexOf("STOP") >= 0) leertemperatura = leerhumedad = leerdistancia = false;
  else if (cmd.indexOf("REANUDAR") >= 0) leertemperatura = leerhumedad = leerdistancia = true;
  else if (cmd.indexOf("PararT") >= 0) leertemperatura = false;
  else if (cmd.indexOf("PararH") >= 0) leerhumedad = false;
  else if (cmd.indexOf("PararD") >= 0) leerdistancia = false;
  else if (cmd.indexOf("IniciarT") >= 0) leertemperatura = true;
  else if (cmd.indexOf("IniciarH") >= 0) leerhumedad = true;
  else if (cmd.indexOf("IniciarD") >= 0) leerdistancia = true;

  // --- CONTROL DEL SERVO MANUAL ---
  else if (cmd.startsWith("DIR:")) {
    int ang = cmd.substring(4).toInt();
    if (ang < 0) ang = 0;
    if (ang > 180) ang = 180;
    servo.write(ang);
    anguloActual = ang;
  }
}
