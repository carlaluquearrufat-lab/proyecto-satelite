#include <SoftwareSerial.h>
int i;
#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySerial(10, 11); // RX, TX 
void setup() {
   pinMode(4, OUTPUT);
   mySerial.begin(9600);
   dht.begin();
   mySerial.println("Empezamos");
   i=1;
   pinMode(6, OUTPUT);
   // Inicialización del sensor

}
void loop() {
   delay (3000);
   
   mySerial.println(i);
   i=i+1;
   float h = dht.readHumidity();
   float t = dht.readTemperature();
   digitalWrite(4, HIGH);
   delay(500);
   digitalWrite (4, LOW);
   
   if (isnan(h) || isnan(t)){
      mySerial.println("Error al leer el sensor DHT11");
      digitalWrite(6, HIGH);
      delay(500);
      digitalWrite (6, LOW);
   
   }else {
      mySerial.print("H: ");
      mySerial.println(h);
      mySerial.print("T: ");
      mySerial.print(t);
   
   

   }
}