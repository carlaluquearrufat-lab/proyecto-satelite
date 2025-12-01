#include <Servo.h>
#include <SoftwareSerial.h>
#include <DHT.h>

// ----- PINES -----
const int TRIG = 8;
const int ECO = 9;
const int LED = 3;
const int ledExito = 4;
const int ledError = 6;

const int DHTPIN = 2;
const int DHTTYPE = DHT11;

// ----- INTERVALOS -----
const unsigned long INTERVALO_SERVO = 20;     // Movimiento suave del servo
const unsigned long INTERVALO_DIST = 80;      // Medición de distancia
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
SoftwareSerial mySerial(10, 11); // RX, TX

int contadorErrores = 0;
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

  Serial.begin(9600);
  mySerial.begin(9600);

  dht.begin();
}

// ----- LOOP -----
void loop() {
  unsigned long ahora = millis();

  // -------------------
  // Envío de número de lectura
  // -------------------
  mySerial.println(numeroEnvio++);
  
  // -------------------
  // Lectura de comandos por SoftwareSerial
  // -------------------
  if (mySerial.available()) {
    static String mensaje = "";
    char c = mySerial.read();
    if (c == '\n') {
      mensaje.trim();
      procesarComando(mensaje);
      mensaje = "";
    } else {
      mensaje += c;
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
      mySerial.println("Error al leer temperatura");
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
      mySerial.println("Error al leer humedad");
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
  }

  // -------------------
  // RADAR AUTOMÁTICO SUAVE - MOVIMIENTO DEL SERVO
  // -------------------
  if (leerdistancia && ahora - tiempoServo >= INTERVALO_SERVO) {
    tiempoServo = ahora;
    anguloActual += incremento * direccion;
    if (anguloActual >= 180) direccion = -1;
    else if (anguloActual <= 0) direccion = 1;
    servo.write(anguloActual);
  }

  // -------------------
  // MEDICIÓN DE DISTANCIA ULTRASÓNICA
  // -------------------
  if (leerdistancia && ahora - tiempoDist >= INTERVALO_DIST) {
    tiempoDist = ahora;
    DISTANCIA = medirDistancia();
    
    // Enviar datos
    if (DISTANCIA == -1.0) DISTANCIA = 0;  // evita -1
    mySerial.print("DATA ");
    mySerial.print(anguloActual);
    mySerial.print(",");
    mySerial.println(DISTANCIA, 1);
  }

  // -------------------
  // LED de envío exitoso
  // -------------------
  parpadeoLed(ledExito, tiempoLedExito, ahora);

  // -------------------
  // Envío de todos los datos
  // -------------------
  mySerial.print("T: "); mySerial.print(TEMPERATURA, 1);
  mySerial.print(" H: "); mySerial.print(HUMEDAD, 1);
  mySerial.print(" Dist(cm): ");
  if (DISTANCIA < 0) mySerial.println("No Echo");
  else mySerial.println(DISTANCIA, 1);
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
    int ang = cmd.substring(4).toInt();  // Leer ángulo enviado desde Python
    if (ang < 0) ang = 0;                // Limitar mínimo
    if (ang > 180) ang = 180;            // Limitar máximo
    servo.write(ang);                     // Mover servo
    anguloActual = ang;  
  }
}
