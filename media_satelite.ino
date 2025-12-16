#include <Servo.h>
#include <SoftwareSerial.h>
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
const unsigned long INTERVALO_ENVIO = 2000;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoEnvio = 0;
unsigned long tiempoLedError = 0;

// -- BUFFER PARA MEDIA --
const int MAX_LECTURAS = 50; 
float bufferTemperaturas[MAX_LECTURAS];
int numLecturasTemp = 0;
float mediaTemperatura = 0;

// -- VARIABLES DE ESTADO --
int anguloActual = 90;
int direccion = 1;
const int incremento = 1;

float DISTANCIA = 0;
float TEMPERATURA = 0;
float HUMEDAD = 0;

bool ISNANT = false;
bool ISNANH = false;

// Flags
bool leertemperatura = true;
bool leerhumedad = true;
bool leerdistancia = true;
bool radarmanual = false;

// Objetos
Servo servo;
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial LoRaSerial(10, 11); // RX, TX

int numeroEnvio = 1;

// ======================================================
// SETUP
// ======================================================
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

    for (int i = 0; i < MAX_LECTURAS; i++) {
        bufferTemperaturas[i] = 0.0;
    }

    Serial.println("INICIANDO SATELITE...");
    delay(2000);

    float tIni = dht.readTemperature();
    if (!isnan(tIni)) {
        TEMPERATURA = tIni;
        bufferTemperaturas[0] = tIni;
        numLecturasTemp = 1;
    }

    Serial.println("SATELITE LISTO.");
}

// ======================================================
// LOOP
// ======================================================
void loop() {

    unsigned long ahora = millis();

    // --------- COMANDOS ---------
    if (LoRaSerial.available()) {
        String cmd = LoRaSerial.readStringUntil('\n');
        procesarComando(cmd);
    }

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        procesarComando(cmd);
    }

    // --------- TEMPERATURA ---------
    if (leertemperatura && ahora - tiempoTemp >= INTERVALO_TEMP) {
        tiempoTemp = ahora;
        float t = dht.readTemperature();

        if (isnan(t)) {
            ISNANT = true;
        } else {
            ISNANT = false;
            TEMPERATURA = t;

            if (numLecturasTemp < MAX_LECTURAS) {
                bufferTemperaturas[numLecturasTemp++] = t;
            } else {
                for (int i = 0; i < MAX_LECTURAS - 1; i++) {
                    bufferTemperaturas[i] = bufferTemperaturas[i + 1];
                }
                bufferTemperaturas[MAX_LECTURAS - 1] = t;
            }
        }
    }

    // --------- HUMEDAD ---------
    if (leerhumedad && ahora - tiempoHum >= INTERVALO_HUM) {
        tiempoHum = ahora;
        float h = dht.readHumidity();
        if (isnan(h)) {
            ISNANH = true;
        } else {
            ISNANH = false;
            HUMEDAD = h;
        }
    }

    // --------- DISTANCIA ---------
    if (leerdistancia && ahora - tiempoDist >= INTERVALO_DIST) {
        tiempoDist = ahora;
        DISTANCIA = medirDistancia();
        if (DISTANCIA < 0) DISTANCIA = 0;
    }

    // --------- ERRORES ---------
    if (ISNANT || ISNANH) {
        parpadeoLed(ledError, tiempoLedError, ahora);
        tone(BUZZER, 1000);
    } else {
        noTone(BUZZER);
        digitalWrite(ledError, LOW);
    }

    // --------- SERVO ---------
    if (!radarmanual && leerdistancia && ahora - tiempoServo >= INTERVALO_SERVO) {
        tiempoServo = ahora;
        anguloActual += incremento * direccion;

        if (anguloActual >= 180) {
            anguloActual = 180;
            direccion = -1;
        } else if (anguloActual <= 0) {
            anguloActual = 0;
            direccion = 1;
        }
        servo.write(anguloActual);
    }

    // --------- ENVÍO AUTOMÁTICO ---------
    if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
        tiempoEnvio = ahora;

        mediaTemperatura = calcularMediaTemperatura();

        String mensaje = "#:" + String(numeroEnvio);

        if (leertemperatura) mensaje += " 1:" + String(TEMPERATURA, 1);
        if (leerhumedad)     mensaje += " 2:" + String(HUMEDAD, 1);
        if (leerdistancia)   mensaje += " 3:" + String(DISTANCIA, 1);

        mensaje += " 5:" + String(mediaTemperatura, 2);
        mensaje += " 4:" + String(anguloActual);

        LoRaSerial.println(mensaje);

        digitalWrite(ledExito, HIGH);
        delay(40);
        digitalWrite(ledExito, LOW);

        numeroEnvio++;
    }
}

// ======================================================
// FUNCIONES
// ======================================================

float calcularMediaTemperatura() {
    if (numLecturasTemp == 0) return TEMPERATURA;

    float suma = 0;
    for (int i = 0; i < numLecturasTemp; i++) {
        suma += bufferTemperaturas[i];
    }
    return suma / numLecturasTemp;
}

float medirDistancia() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    unsigned long duracion = pulseIn(ECO, HIGH, 25000UL);
    if (duracion == 0) return -1.0;
    return duracion / 58.2;
}

void parpadeoLed(int ledPin, unsigned long &marca, unsigned long ahora) {
    if (ahora - marca >= 300) {
        marca = ahora;
        digitalWrite(ledPin, !digitalRead(ledPin));
    }
}

void procesarComando(String cmd) {
    cmd.trim();
    Serial.println("CMD: " + cmd);

    if (cmd.indexOf("S1") >= 0)      leertemperatura = false;
    else if (cmd.indexOf("S2") >= 0) leerhumedad = false;
    else if (cmd.indexOf("S3") >= 0) leerdistancia = false;

    else if (cmd.indexOf("R1") >= 0) leertemperatura = true;
    else if (cmd.indexOf("R2") >= 0) leerhumedad = true;
    else if (cmd.indexOf("R3") >= 0) {
        leerdistancia = true;
        radarmanual = false;
    }

    else if (cmd.startsWith("RM:")) {
        radarmanual = true;
        int ang = cmd.substring(3).toInt();
        anguloActual = constrain(ang, 0, 180);
        servo.write(anguloActual);
    }
}
