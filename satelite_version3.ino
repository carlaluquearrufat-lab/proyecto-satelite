#include <Servo.h>
int TRIG = 8;
int ECO = 9;
int LED = 3;
// Mínimo cambio: usar tipo adecuado para pulseIn
unsigned long DURACION;

const unsigned long ahora= millis();
unsigned long intervalo_dis;
unsigned long intervalo_servo=1000; //Tiempo que tarda en ir de 0-180 gradas y de vuelta a 0 grados. 
unsigned long intervalo_temp=3000;
unsigned long intervalo_hum= 3000;
unsigned long intervalo_led= 1000;

unsigned long dis_anterior=0;
unsigned long servo_anterior=0;
unsigned long temp_anterior=0;
unsigned long hum_anterior=0;
unsigned long led_anterior=0;

const int incremento= 10; //grados que cambia el servo
int direccion= 1; // 1 derecha, -1 izquierda
int anguloActual= 90;

float DISTANCIA;
String DISTANCIAStr;
String TEMPERATURA;
String HUMEDAD;

#include <SoftwareSerial.h>
SoftwareSerial mySerial(10,11); //RX,TX
Servo servo;

#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const int ledErrorT = 4;
const int ledErrorH = 6;

int i=0;

bool leertemperatura= true;
bool leerhumedad= true;
bool leerdistancia= true;
bool leerdistanciamanual= false;

bool ISNANT= false;
bool ISNANH= false;

void setup() {
  servo.attach(13); //Pin del servo
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);    // <-- CAMBIO: ECO debe ser INPUT
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW); // asegurar TRIG en LOW al inicio
  Serial.begin(9600);   
  mySerial.begin(9600);
  dht.begin();
  servo.write(90); //Posición neutral

  pinMode(ledErrorT, OUTPUT);
  pinMode(ledErrorH, OUTPUT);

  i=1;
  pinMode(4, OUTPUT);
}

  String linea = "";

void loop() {
  intervalo_dis = intervalo_servo;
  const unsigned long ahora= millis();

  mySerial.println(i);
  i=i+1;

  if (mySerial.available()){
    char c= mySerial.read();
    if  (c == '\n'){
      linea.trim();
      instrucciones (linea);
      linea= "";
    } 
    else {
      linea += c;
    }
  }

  if (leertemperatura){
    if (ahora - temp_anterior>= intervalo_temp){
      temp_anterior= ahora;
      TEMPERATURA= medirTemperatura();
      if (isnan(TEMPERATURA)){
        mySerial.println("Error al leer la humedad");
        encenderled(ledErrorT);
        apagarled(ledErrorT);
      } 
    }
  }

  if (!leertemperatura){
    TEMPERATURA= "Lectura temperatura detenida";
  }

  if (leerhumedad){
    if (ahora - hum_anterior>= intervalo_hum){
      hum_anterior= ahora;
      HUMEDAD= medirHumedad();
      if (isnan(HUMEDAD)){
        mySerial.println("Error al leer la humedad");
        encenderled(ledErrorH);
        apagarled(ledErrorH);
      } 
    }
  }

  if (!leerhumedad){
    HUMEDAD= "Lectura humedad detenida";
  }
  
  if (leerdistancia){
    if (ahora - servo_anterior>= intervalo_servo){
      servo_anterior= ahora; 
      servo.write(anguloActual);
      anguloActual += incremento * direccion;
      //Cambiar de sentido en los extremos
      if (anguloActual >= 180){
        anguloActual= 180;
        direccion = -1;
      }
      else if (anguloActual <= 0){
        anguloActual = 0;
        direccion= 1;
      }
    }
    if (ahora - dis_anterior >= intervalo_servo){
      dis_anterior= ahora;
      DISTANCIA= medirDistancia();
    }
  }

  String mensaje;

  if (leerdistanciamanual){
    if (c== '\n'){
      procesarComando(mensaje);
      mensaje = "";
    } 
    else {
      mensaje += c;
    }
  } 
  if (!leerdistancia && !leerdistanciamanual){
    DISTANCIAStr= "Lectura distancia detenida";
  }

  mySerial.print(" T: "); mySerial.print(TEMPERATURA);
  mySerial.print(" H: "); mySerial.print(HUMEDAD);
  mySerial.print(" Dist (cm): ");
  if (DISTANCIA == -1.0) {
    mySerial.println("No Echo");
  }
  else {
    DISTANCIAStr= DISTANCIA;
    mySerial.println(DISTANCIAStr);
  }
}
 

//Función para clasificar las instrucciones de la interfaz (parar,reanudar...)
void instrucciones(String linea){
    if (linea.indexOf("STOP") >= 0){
      leertemperatura = leerhumedad = leerdistancia = false;
    }
    if (linea.indexOf("REANUDAR") >= 0){
      leertemperatura = leerhumedad = leerdistancia = true;
    }
    if (linea.indexOf("PararT") >= 0){
      leertemperatura= false;
    }
    if (linea.indexOf("PararH") >= 0){
      leerhumedad= false;
    }
    if (linea.indexOf("PararD") >= 0){
      leerdistancia= false;
    }
    if (linea.indexOf("IniciatT") >= 0){
      leertemperatura= true;
    }
    if (linea.indexOf("IniciarH") >= 0){
      leerhumedad= true;
    }
    if (linea.indexOf("IniciarD") >= 0){
      leerdistancia= true; 
    }
    if (linea.indexOf("RadarManual") >= 0){
    leerdistanciamanual= true;
    leerdistancia= false;
    }
    else {
      c= linea;
    }
}
//Función para devolver la temperatura leída cada i tiempo
String medirTemperatura(){
  float t = (dht.readTemperature();
  if (isnan(t)) {
    return "Error temperatura"
  } else{
    return String(t,1); //solo un decimal
  }
}

//Función para devolver la humedad leída cada i tiempo
String medirHumedad(){
  float h = (dht.readHumidity();
  if (isnan(h)) {
    return "Error humedad"
  } else{
    return String(h,1); //solo un decimal
  }
}

//Función mínima para medir distancia con timeout; devuelve -1.0 si timeout
float medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  DURACION = pulseIn(ECO, HIGH, 30000UL); // timeout 30 ms

  if (DURACION == 0) {
    return -1.0;
  }  else {
  return DURACION / 58.2;
  }
}

//Función para encender un led y a los 500 milisegundos se apaga.
void encenderled ( const int Led){
  unsigned long ahora= millis();
  if (ahora - led_anterior>= intervalo_led/2){
    led_anterior=ahora;
    digitalWrite(Led, HIGH);
    }
  }
void apagarled (const int Led){
  unsigned long ahora= millis();
  if (ahora - led_anterior>= intervalo_led){
    led_anterior=ahora;
    digitalWrite(Led, LOW);
  }
}

void procesarComando(String cmd) {
  if (cmd.startsWith("DIR:")) {
    int dir = cmd.substring(4).toInt();
    if (dir == 0) {
      // Izquierda
      servo.write(0);
    } 
    else if (dir == 1) {
      // Derecha
      servo.write(180);
    } 
    else if (dir == 2) {
      // Neutro
      servo.write(90);
    }
  }
}
