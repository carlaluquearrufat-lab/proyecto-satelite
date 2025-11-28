#include <Servo.h>
int TRIG = 8;
int ECO = 9;
int LED = 3;
// Mínimo cambio: usar tipo adecuado para pulseIn
unsigned long DURACION;
float DISTANCIA;
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10,11); //RX,TX
Servo servo;

#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const int ledExito = 4;
const int ledError = 6;

int i = 1;
int contadorError = 0;

void setup() {
  servo.attach(13); 
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);    // <-- CAMBIO: ECO debe ser INPUT
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW); // asegurar TRIG en LOW al inicio
  Serial.begin(9600);   
  mySerial.begin(9600);
  dht.begin();

  pinMode(ledExito, OUTPUT);
   pinMode(ledError, OUTPUT);

  i=1;
  pinMode(4, OUTPUT);
}

void loop() {

   if (mySerial.available()) {
    String cmd = mySerial.readStringUntil('\n');
    cmd.trim();
    Serial.println(cmd);
     
  mySerial.println(i);
  i=i+1;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)){
    mySerial.println("Error al leer el sensor DHT11");
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite (6, LOW);
  } else {

    
    for (int angulo = 0; angulo <= 180; angulo += 10) {
      servo.write(angulo);
      delay(600); // esperar a que el servo llegue antes de medir

      DISTANCIA = medirDistancia();

      // Enviar H, T, Angulo, Distancia en una sola línea
      mySerial.print("H: "); mySerial.print(h);
      mySerial.print(" T: "); mySerial.print(t);
      mySerial.print(" Angulo: "); mySerial.print(angulo);
      mySerial.print(" Dist (cm): ");
      digitalWrite(ledExito, HIGH);
      delay(500);
      digitalWrite(ledExito, LOW);
      if (DISTANCIA == -1.0) mySerial.println("No echo (timeout)");
      else {
        mySerial.println(DISTANCIA);
      }

    }

    // Regresar de 180° a 0°
    for (int angulo = 180; angulo >= 0; angulo -= 10) {
      servo.write(angulo);
      delay(600); // esperar a que el servo llegue antes de medir

      DISTANCIA = medirDistancia();

      // Enviar H, T, Angulo, Distancia en una sola línea
      mySerial.print("H: "); mySerial.print(h);
      mySerial.print(" T: "); mySerial.print(t);
      mySerial.print(" Angulo: "); mySerial.print(angulo);
      mySerial.print(" Dist (cm): ");
      digitalWrite(ledExito, HIGH);
      delay(500);
      digitalWrite(ledExito, LOW);
      if (DISTANCIA == -1.0) mySerial.println("No echo (timeout)");
      else {
        mySerial.println(DISTANCIA);
      }

  
    }

  } // fin else DHT OK
}

// Función mínima para medir distancia con timeout; devuelve -1.0 si timeout
float medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  DURACION = pulseIn(ECO, HIGH, 30000UL); // timeout 30 ms

  if (DURACION == 0) {
    return -1.0;
  } else {
    return DURACION / 58.2;
  }
  

}
