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
const unsigned long INTERVALO_ENVIO = 2000; // envío LoRa
const unsigned long INTERVALO_LED = 500;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoEnvio = 0;
unsigned long tiempoLedExito = 0;
unsigned long tiempoLedError = 0;

int anguloActual = 90;
int direccion = 1;
int contador=0;
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
            LoRaSerial.println("1!");
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
            LoRaSerial.println("2!");
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
        contador= contador +1;
    } else {
        noTone(BUZZER);
        contador=0;
    }
    if (contador>=3){
        LoRaSerial.println("1!2");
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

    // -------------------
    // Envío de datos por LoRa cada INTERVALO_ENVIO
    // -------------------
    if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
        tiempoEnvio = ahora;

        String radar = "DATA " + String(anguloActual) + "," + String(DISTANCIA,1);
        LoRaSerial.println(radar);  // asegura que la interfaz detecte la línea DATA
        
        String mensaje = "#:" + String(numeroEnvio++) +
                         "1:" + String(TEMPERATURA,1) +
                         "2:" + S#tring(HUMEDAD,1) +
                         "3:" + String(DISTANCIA,1) +
                         "4:" + String(anguloActual);
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
    if (cmd.indexOf("S") >= 0) 
        leertemperatura = leerhumedad = leerdistancia = false;
    else if (cmd.indexOf("R") >= 0) 
        leertemperatura = leerhumedad = leerdistancia = true;
    else if (cmd.indexOf("S1") >= 0) 
        leertemperatura = false;
    else if (cmd.indexOf("S2") >= 0) 
        leerhumedad = false;
    else if (cmd.indexOf("S3") >= 0) 
        leerdistancia = false;
    else if (cmd.indexOf("R1") >= 0) 
        leertemperatura = true;
    else if (cmd.indexOf("R2") >= 0) 
        leerhumedad = true;
    else if (cmd.indexOf("R3") >= 0) 
        leerdistancia = true;
    else if (cmd.startsWith("DIR:")) {
        int ang = cmd.substring(4).toInt();
        if (ang < 0) ang = 0;
        if (ang > 180) ang = 180;
        servo.write(ang);
        anguloActual = ang;  
    }
}
