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
const unsigned long INTERVALO_ENVIO = 2000; // Envío de telemetría automática
const unsigned long INTERVALO_LED = 500;

// ----- VARIABLES -----
unsigned long tiempoServo = 0;
unsigned long tiempoDist = 0;
unsigned long tiempoTemp = 0;
unsigned long tiempoHum = 0;
unsigned long tiempoEnvio = 0;
unsigned long tiempoLedExito = 0;
unsigned long tiempoLedError = 0;

// -- BUFFER PARA MEDIA --
const int MAX_LECTURAS = 50; 
float bufferTemperaturas[MAX_LECTURAS];
int numLecturasTemp = 0;
float mediaTemperatura = 0;

// -- VARIABLES DE ESTADO --
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
bool radarmanual = false;
bool enviarMedia = false; 

// Objetos
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

    Serial.begin(9600);     // USB
    LoRaSerial.begin(9600); // LoRa

    dht.begin();
    
    // Inicializar buffer a 0
    for(int i=0; i<MAX_LECTURAS; i++){
        bufferTemperaturas[i] = 0.0;
    }

    Serial.println("INICIANDO SENSORES...");
    // MEJORA 1: Esperar un poco y hacer primera lectura para evitar ceros iniciales
    delay(2000); 
    float tIni = dht.readTemperature();
    if (!isnan(tIni)) {
        TEMPERATURA = tIni;
        bufferTemperaturas[0] = tIni;
        numLecturasTemp = 1;
        Serial.print("Lectura inicial Temp: "); Serial.println(tIni);
    } else {
        Serial.println("Error lectura inicial DHT");
    }
    
    Serial.println("SATELITE INICIADO. Esperando comandos...");
}

// ----- LOOP -----
void loop() {
    unsigned long ahora = millis();

    // -------------------
    // 1. Lectura de comandos (LoRa y Serial USB)
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

    // -------------------
    // 2. Lectura de Temperatura (Buffer Circular)
    // -------------------
    if (leertemperatura && (ahora - tiempoTemp >= INTERVALO_TEMP)) {
        tiempoTemp = ahora;
        float t = dht.readTemperature();
        
        if (isnan(t)) {
            ISNANT = true;
            Serial.println("Error leyendo Temp (NaN)");
        } else {
            TEMPERATURA = t;
            ISNANT = false;

            // Debug en USB
            // Serial.print("T: "); Serial.println(t);

            // --- AGREGAR AL BUFFER ---
            if (numLecturasTemp < MAX_LECTURAS) {
                bufferTemperaturas[numLecturasTemp++] = t;
            } else {
                // Buffer lleno: desplazar izquierda
                for (int i = 0; i < MAX_LECTURAS - 1; i++) {
                    bufferTemperaturas[i] = bufferTemperaturas[i+1];
                }
                bufferTemperaturas[MAX_LECTURAS-1] = t;
            }
        }
    }

    // -------------------
    // 3. Lectura de Humedad
    // -------------------
    if (leerhumedad && (ahora - tiempoHum >= INTERVALO_HUM)) {
        tiempoHum = ahora;
        float h = dht.readHumidity();
        if (isnan(h)) {
            ISNANH = true;
        } else {
            HUMEDAD = h;
            ISNANH = false;
        }
    }

    // -------------------
    // 4. Gestión de Errores
    // -------------------
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

    // -------------------
    // 5. Movimiento Servo (Radar)
    // -------------------
    if (!radarmanual && leerdistancia && (ahora - tiempoServo >= INTERVALO_SERVO)) {
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

    // -------------------
    // 6. Medición Distancia
    // -------------------
    if (leerdistancia && (ahora - tiempoDist >= INTERVALO_DIST)) {
        tiempoDist = ahora;
        DISTANCIA = medirDistancia();
        if (DISTANCIA < 0) DISTANCIA = 0; 
    }

    // -------------------
    // 7. Envío Automático (Telemetría)
    // -------------------
    if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
        tiempoEnvio = ahora;

        String mensaje = "#:" + String(numeroEnvio);
        
        if (leertemperatura) mensaje += " 1:" + String(TEMPERATURA, 1);
        if (leerhumedad)     mensaje += " 2:" + String(HUMEDAD, 1);
        if (leerdistancia)   mensaje += " 3:" + String(DISTANCIA, 1);
        
        // --- MEJORA 2: ENVIO UNICO DE MEDIA ---
        // Si se solicitó la media, se añade y SE RESETEA la bandera
        if (enviarMedia) {
            mensaje += " 5:" + String(mediaTemperatura, 2); // 2 decimales para precisión
            enviarMedia = false; // IMPORTANTE: Apagamos bandera para no repetir
            Serial.println("Media enviada via LoRa.");
        }
        
        mensaje += " 4:" + String(anguloActual);

        LoRaSerial.println(mensaje);
        
        if (!ISNANT && !ISNANH) {
             digitalWrite(ledExito, HIGH);
             delay(50); 
             digitalWrite(ledExito, LOW);
        }
        
        numeroEnvio++;
    }
}

// ----- FUNCIONES AUXILIARES -----

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
    Serial.println("CMD RECIBIDO: " + cmd); // Debug

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
        ang = constrain(ang, 0, 180);
        servo.write(ang);
        anguloActual = ang;  
    }
    
    // --- CALCULO DE MEDIA ---
    else if (cmd == "M") {
        float suma = 0;
        Serial.print("Calculando media de "); 
        Serial.print(numLecturasTemp); 
        Serial.println(" lecturas.");

        if (numLecturasTemp > 0) {
            for (int i = 0; i < numLecturasTemp; i++) {
                suma += bufferTemperaturas[i];
            }
            mediaTemperatura = suma / (float)numLecturasTemp;
        } else {
            // Si el buffer está vacío, intentamos usar la lectura actual
            // o 0 si no hay ninguna válida.
            if(TEMPERATURA != 0) mediaTemperatura = TEMPERATURA;
            else mediaTemperatura = 0.0;
        }
        
        Serial.print("Resultado Media: ");
        Serial.println(mediaTemperatura);
        
        enviarMedia = true; // Activa la bandera para el próximo envío
    }
}
