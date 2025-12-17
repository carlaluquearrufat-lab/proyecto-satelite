#include <Servo.h>
#include <SoftwareSerial.h>

#include <DHT.h>

// ----- PINES -----
const int TRIG = 8;
const int ECO = 9;
const int LED = 3;       // LED General
const int ledExito = 4;  // Parpadea al enviar OK
const int ledError = 6;  // Parpadea si hay error sensor
const int BUZZER = 7;
const int DHTPIN = 2;
const int DHTTYPE = DHT11;

// ----- INTERVALOS -----
const unsigned long INTERVALO_SERVO = 20;
const unsigned long INTERVALO_DIST = 80;
const unsigned long INTERVALO_TEMP = 3000;
const unsigned long INTERVALO_HUM = 3000;
const unsigned long INTERVALO_ENVIO = 3000; 
const unsigned long INTERVALO_LED = 500;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoEnvio = 0;
unsigned long tiempoLedExito = 0;
unsigned long tiempoLedError = 0;
int ultimoAnguloEscrito = 90;

// ----- RADAR -----
enum RadarModo { RADAR_AUTO, RADAR_MANUAL, RADAR_STOP };
RadarModo radarModo = RADAR_AUTO;

// ----- BUFFER PARA MEDIA -----
const int MAX_LECTURAS = 50; 
float bufferTemperaturas[MAX_LECTURAS];
int numLecturasTemp = 0;
float mediaTemperatura = 0;


// ----- VARIABLES DE ESTADO -----
int anguloActual = 90;
int direccion = 1;
int contador = 0;
const int incremento = 1;

float DISTANCIA = 0;
float TEMPERATURA = 0;
float HUMEDAD = 0;

bool ISNANT = false;
bool ISNANH = false;

// Flags de control
bool leertemperatura = true;
bool leerhumedad = true;
bool leerdistancia = true;


// ----- OBJETOS -----
Servo servo;
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial LoRaSerial(10, 11); // RX, TX

int numeroEnvio = 1;

// ----- PROTOTIPOS DE FUNCIONES -----
void escribirServo(int ang, bool fuerza=false) {
    if (radarModo == RADAR_MANUAL && fuerza) {
        // Modo manual: siempre escribir
        servo.write(ang);
        ultimoAnguloEscrito = ang; // actualizar para consistencia
    } 
    else if (radarModo == RADAR_AUTO) {
        // Modo automático: escribir solo si cambió
        if (ang != ultimoAnguloEscrito) {
            servo.write(ang);
            ultimoAnguloEscrito = ang;
        }
    }
    // Radar STOP: no escribir
}

void moverManual(int ang);

void actualizarServo();
float medirDistancia();
void parpadeoLed(int ledPin, unsigned long &marca, unsigned long ahora);
void procesarComando(String cmd);

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
    // --- Lectura inicial para evitar ceros ---
    delay(2000); // tiempo recomendado DHT11
    for(int i=0; i<MAX_LECTURAS; i++){
        bufferTemperaturas[i] = 0.0;
    }
    float tIni = dht.readTemperature();
    if (!isnan(tIni)) {
        TEMPERATURA = tIni;
        bufferTemperaturas[0] = tIni;
        numLecturasTemp = 1;
        Serial.print("Lectura inicial Temp: ");
        Serial.println(tIni);
    } else {
        Serial.println("Error lectura inicial DHT");
    }

    // Inicializar buffer a 0
    
    Serial.println("SATELITE INICIADO. Esperando comandos...");
}

// ----- LOOP -----
void loop() {
    actualizarServo();
    unsigned long ahora = millis();

    // 1. Lectura de comandos (LoRa y Serial USB)
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
    
    if (Serial.available()) {
        static String mensajeSerial = "";
        char c = Serial.read();
        if (c == '\n') {
            mensajeSerial.trim();
            procesarComando(mensajeSerial);
            mensajeSerial = "";
        } else {
            mensajeSerial += c;
        }
    }

    // 2. Lectura de Temperatura
    if (leertemperatura && (ahora - tiempoTemp >= INTERVALO_TEMP)) {
        tiempoTemp = ahora;
        float t = dht.readTemperature();
        if (isnan(t)) ISNANT = true;
        else {
            TEMPERATURA = t;
            ISNANT = false;
            if (numLecturasTemp < MAX_LECTURAS) bufferTemperaturas[numLecturasTemp++] = t;
            else {
                for (int i = 0; i < MAX_LECTURAS-1; i++) bufferTemperaturas[i] = bufferTemperaturas[i+1];
                bufferTemperaturas[MAX_LECTURAS-1] = t;
            }
        }
    }

    // 3. Lectura de Humedad
    if (leerhumedad && (ahora - tiempoHum >= INTERVALO_HUM)) {
        tiempoHum = ahora;
        float h = dht.readHumidity();
        if (isnan(h)) ISNANH = true;
        else { HUMEDAD = h; ISNANH = false; }
    }

    // 4. Gestión de Errores
    if (ISNANT || ISNANH) {
        parpadeoLed(ledError, tiempoLedError, ahora);
        digitalWrite(ledExito, LOW);
        if (ISNANT && ISNANH) {
            tone(BUZZER, 1000);
            contador++;
        } else {
            noTone(BUZZER);
            contador = 0;
        }
    } else {
        noTone(BUZZER);
        digitalWrite(ledError, LOW);
        contador = 0;
    }
    if (contador >= 3) {
        LoRaSerial.println("1!2");
        contador = 0; 
    }

    // 6. Medición Distancia
    if (leerdistancia && (ahora - tiempoDist >= INTERVALO_DIST)) {
        tiempoDist = ahora;
        DISTANCIA = medirDistancia();
        if (DISTANCIA < 0) DISTANCIA = 0; 
    }

    // 7. Envío Automático
    if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
        tiempoEnvio = ahora;
        mediaTemperatura = calcularMediaTemperatura();

        String mensaje = "#:" + String(numeroEnvio);

                if (leertemperatura) mensaje += " 1:" + String(TEMPERATURA,1);
                if (leerhumedad)     mensaje += " 2:" + String(HUMEDAD,1);
                if (leerdistancia)   mensaje += " 3:" + String(DISTANCIA,1);

                mensaje += " 5:" + String(mediaTemperatura, 2);
                mensaje += " 4:" + String(anguloActual);

        LoRaSerial.println(mensaje);

        // LED Éxito
        digitalWrite(ledExito, HIGH);
        delay(40);
        digitalWrite(ledExito, LOW);
        numeroEnvio++;
    }
}

// ----- FUNCIONES AUXILIARES -----

void moverManual(int ang) {
    anguloActual = constrain(ang, 0, 180);
    escribirServo(anguloActual, true);
}

void actualizarServo() {
    if (radarModo != RADAR_AUTO) return;

    unsigned long ahora = millis();
    if (ahora - tiempoServo < INTERVALO_SERVO) return;
    tiempoServo = ahora;

    anguloActual += incremento * direccion;
    if (anguloActual >= 180) { anguloActual = 180; direccion = -1; }
    else if (anguloActual <= 0) { anguloActual = 0; direccion = 1; }

    escribirServo(anguloActual); // automático
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

    // =========================
    // CONTROL DE SENSORES
    // =========================
    if (cmd == "S1") {
        leertemperatura = false;
    }
    else if (cmd == "S2") {
        leerhumedad = false;
    }
    else if (cmd == "S3") {
        leerdistancia = false;
    }
    else if (cmd == "R1") {
        leertemperatura = true;
    }
    else if (cmd == "R2") {
        leerhumedad = true;
    }
    else if (cmd == "R3") {
        leerdistancia = true;
    }

    // =========================
    // CONTROL RADAR / SERVO
    // =========================
    static bool servoActivo = true; // recuerda si el servo está attach

    // Radar parado
    if (cmd == "RS") {
        radarModo = RADAR_STOP;
        if (servoActivo) {
            servo.detach();        // Mantiene posición y evita glitches
            servoActivo = false;
        }
    }
    // Radar automático
    else if (cmd == "RA" || cmd == "RR") {
        radarModo = RADAR_AUTO;
        if (!servoActivo) {
            servo.attach(13);      // Reactivar servo si estaba detach
            servoActivo = true;
        }
    }
    // Radar manual (RM:angulo)
    else if (cmd.startsWith("RM:")) {
        radarModo = RADAR_MANUAL;
        int ang = cmd.substring(3).toInt();
        anguloActual = constrain(ang, 0, 180);   // fijar ángulo
        escribirServo(anguloActual, true);       // ⚡ fuerza el movimiento manual
        if (!servoActivo) {
            servo.attach(13);                    // asegurar que el servo esté activo
            servoActivo = true;
        }
    }

    // =========================
    // MEDIA TEMPERATURA
    // =========================

}
float calcularMediaTemperatura() {
    if (numLecturasTemp == 0) return TEMPERATURA;

    float suma = 0.0;
    for (int i = 0; i < numLecturasTemp; i++) {
        suma += bufferTemperaturas[i];
    }
    return suma / numLecturasTemp;
}

