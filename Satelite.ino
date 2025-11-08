#include <SoftwareSerial.h>
int i;
int contador=0;
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
  
   while ( !isnan(h) && !isnan(t)){
      digitalWrite(4, HIGH);
      delay(500);
      digitalWrite (4, LOW);
      }
   
   if (isnan(h) || isnan(t)){
      digitalWrite(4, LOW);
      mySerial.println("Error al leer el sensor DHT11");
      contador++;
      while (contador > 0){
      digitalWrite(6, HIGH);
      delay(500);
      digitalWrite (6, LOW);
      }
      //Si se capturan mal los datos más de tres veces enviamos mensaje para saltar alarma en tierra
      if (contador >= 3){
         Serial.print ("Fallo");
      }
   }
   else {
      contador= 0;
      mySerial.print("H: ");
      mySerial.print(h);
      mySerial.print("T: ");
      mySerial.println(t);
   }
}

