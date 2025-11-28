#include <Servo.h>
#include <SoftwareSerial.h>
#include <DHT.h>

// Pines
int TRIG = 8;
int ECO = 9;
int LED = 3;

unsigned long DURACION;

// Intervalos
unsigned long intervalo_servo = 1000;
unsigned long intervalo_temp  = 3000;
unsigned long intervalo_hum   = 3000;
unsigned long intervalo_led   = 500;

unsigned long dis_anterior   = 0;
unsigned long servo_anterior = 0;
unsigned long temp_anterior  = 0;
unsigned long hum_anterior   = 0;
unsigned long led_anterior_Exito = 0;
unsigned long led_anterior_Error = 0;

const int incremento = 10;
int direccion = 1;
int anguloActual = 90;

// Variables globales
float DISTANCIA;
String TEMPERATURA;
String HUMEDAD;

SoftwareSerial mySerial(10, 11); //RX,TX
Servo servo;

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int ledExito = 4;
const int ledError = 6;

int i = 0;
int contador = 0;

bool leertemperatura = true;
bool leerhumedad = true;
bool leerdistancia = true;
bool leerdistanciamanual = false;

bool ISNANT = false; 
bool ISNANH = false;

// Buffer lectura serie
String linea = "";

void setup() {
  servo.attach(13);
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW);

  Serial.begin(9600);
  mySerial.begin(9600);
  
  dht.begin();

  servo.write(90);

  pinMode(ledExito, OUTPUT);
  pinMode(ledError, OUTPUT);

  i = 1;
}

void loop() {
  unsigned long ahora = millis();

  // Numero de envio
  mySerial.println(i++);
  
  // Lectura comandos
  if (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      linea.trim();
      instrucciones(linea);
      linea = "";
    } else {
      linea += c;
    }
  }

  // TEMPERATURA
  if (leertemperatura) {
    if (ahora - temp_anterior >= intervalo_temp) {
      temp_anterior = ahora;
      TEMPERATURA = medirTemperatura();
      
      if (TEMPERATURA == "Error") {
        ISNANT = true;
        contador ++;
      } else {
        ISNANT = false;
      }
      if (ISNANT) {
        mySerial.println("Error al leer temperatura");
      }
    } else {
    TEMPERATURA = "Lectura detenida";
    }
  }

  // HUMEDAD
  if (leerhumedad) {
    if (ahora - hum_anterior >= intervalo_hum) {
      hum_anterior = ahora;
      HUMEDAD = medirHumedad();
      
      if (HUMEDAD == "Error") {
        ISNANH = true;
        contador ++;
      } else {
        ISNANH = false;
      }
      if (ISNANH) {
        mySerial.println("Error al leer humedad");
      }
    } else {
      HUMEDAD = "Lectura detenida";
    }
  }

  //FALLO EN TEMPERATURA Y HUMEDAD
  if (ISNANT && ISNANH){
    parpadeoLed(ledError, led_anterior_Error, ahora);
  }
  //si hay mas de 5 errores en la lectrua de temperatura o humedad
  if (contador >= 5){
    mySerial.println("Fallo TH");
  } 
    
  // RADAR 
  if (leerdistancia) {
    if (ahora - servo_anterior >= intervalo_servo) {
      servo_anterior = ahora;

      servo.write(anguloActual);
      anguloActual += incremento * direccion;

      if (anguloActual >= 180) {
        anguloActual = 180;
        direccion = -1;
      }
      else if (anguloActual <= 0) {
        anguloActual = 0;
        direccion = 1;
      }
    }

    if (ahora - dis_anterior >= intervalo_servo) {
      dis_anterior = ahora;
      DISTANCIA = medirDistancia();
    }
  }

  // RADAR MANUAL
  if (leerdistanciamanual) {
    if (mySerial.available()) {
      char c = mySerial.read();
      if (c == '\n') {
        procesarComando(linea);
        linea = "";
      } else {
        linea += c;
      }
    }
  }
    
  // ENVÍO DE DATOS
  parpadeoLed(ledExito, led_anterior_Exito, ahora);
  mySerial.print(" T: "); mySerial.print(TEMPERATURA);
  mySerial.print(" H: "); mySerial.print(HUMEDAD);
  mySerial.print(" Dist (cm): ");

  if (DISTANCIA == -1.0) {
    mySerial.println("No Echo");
  } else {
    mySerial.println(DISTANCIA, 1);
  }
}

// FUNCIONES
void instrucciones(String linea) {

  if (linea.indexOf("STOP") >= 0) {
    leertemperatura = leerhumedad = leerdistancia = false;
  }
  
  if (linea.indexOf("REANUDAR") >= 0) {
    leertemperatura = leerhumedad = leerdistancia = true;
  }

  if (linea.indexOf("PararT") >= 0) {
    leertemperatura = false;
  }
  if (linea.indexOf("PararH") >= 0){
    leerhumedad   = false;
  }
  if (linea.indexOf("PararD") >= 0) {
    leerdistancia = false;
  }
  if (linea.indexOf("IniciarT") >= 0){
    leertemperatura = true;
  }
  if (linea.indexOf("IniciarH") >= 0){
    leerhumedad     = true;
  }
  if (linea.indexOf("IniciarD") >= 0){
    leerdistancia   = true;
  }
  if (linea.indexOf("RadarManual") >= 0) {
    leerdistanciamanual = true;
    leerdistancia = false;
  }
}

String medirTemperatura() {
  float t = dht.readTemperature();
  if (isnan(t)) return "Error";
  return String(t, 1);
}

String medirHumedad() {
  float h = dht.readHumidity();
  if (isnan(h)) return "Error";
  return String(h, 1);
}

float medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  DURACION = pulseIn(ECO, HIGH, 30000UL);

  if (DURACION == 0) return -1.0;
  return DURACION / 58.2;
}

void parpadeoLed(int ledPin, unsigned long &marca, unsigned long ahora) {
  if (ahora - marca >= intervalo_led) {
    marca = ahora;
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

void procesarComando(String cmd) {
  if (cmd.startsWith("DIR:")) {
    int dir = cmd.substring(4).toInt();
    if (dir == 0) servo.write(0);
    else if (dir == 1) servo.write(180);
    else if (dir == 2) servo.write(90);
  }
}
