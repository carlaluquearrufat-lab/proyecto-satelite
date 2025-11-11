#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySerial(10, 11); // RX, TX 

const int ledExito = 4;
const int ledError = 6;

int i = 1;
int contador = 0;
int contadorError = 0;

void setup() {
   pinMode(ledExito, OUTPUT);
   pinMode(ledError, OUTPUT);
   mySerial.begin(9600);
   dht.begin();
   mySerial.println("Empezamos");
}

void loop() {
   delay(3000);

   mySerial.println(i);
   i++;

   float h = dht.readHumidity();
   float t = dht.readTemperature();

   // --- Clasificación de datos ---
   if (!isnan(h) && !isnan(t)) {
       // Datos correctos
       contador = 0;

       // LED éxito parpadea (igual que antes)
       digitalWrite(ledExito, HIGH);
       delay(500);
       digitalWrite(ledExito, LOW);

       // Enviar datos clasificados
       mySerial.println("1:" + String(t)); // Temperatura
       mySerial.println("2:" + String(h)); // Humedad
   }
   else {
       // Datos incorrectos
       contadorError++;
       digitalWrite(ledExito, LOW);

       // LED error parpadea (igual que antes)
       digitalWrite(ledError, HIGH);
       delay(500);
       digitalWrite(ledError, LOW);

       // CAMBIO: enviar mensaje de error clasificado
       mySerial.println("4:"); // Error de captura

       // Si hay 3 errores consecutivos, enviar "Fallo"
       contador++;
       if (contador >= 3) {
           mySerial.println("Fallo");
       }
   }
}
