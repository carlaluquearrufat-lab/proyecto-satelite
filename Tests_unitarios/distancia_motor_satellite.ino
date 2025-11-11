#include <Servo.h>
Servo servo;
int TRIG = 10;
int ECO = 9;
int LED = 3;
// Mínimo cambio: usar tipo adecuado para pulseIn
unsigned long DURACION;
float DISTANCIA;
//modo automatico
int angulo = 0;
bool subiendo=true;
//modo manual
bool modoManual = false;
int anguloManual = 0;

unsigned long tiempoAnterior = 0;
const int intervaloServo = 50; 

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);    // <-- CAMBIO: ECO debe ser INPUT
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW); // asegurar TRIG en LOW al inicio
  servo.attach (13);
  Serial.begin(9600);
  
}

void loop(){
  if (Serial.available() > 0) {
    int comando = Serial.parseInt(); // leer ángulo
    if (comando >= 0 && comando <= 180) {
      anguloManual = comando;
      modoManual = true;             // activar modo manual
      servo.write(anguloManual);
      Serial.print("Servo movido a: ");
      Serial.println(anguloManual);
    }
  }

  //Medir distancia 
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  DURACION = pulseIn(ECO, HIGH, 30000UL);
  if (DURACION == 0) {
    Serial.println("No echo (timeout)");
    digitalWrite(LED, LOW);
  } else {
    DISTANCIA = DURACION / 58.2;
    Serial.print("Distancia (cm): ");
    Serial.println(DISTANCIA);

    digitalWrite(LED, (DISTANCIA < 20.0) ? HIGH : LOW);
  }

  // Motor automático
  if (!modoManual) {
    unsigned long tiempoActual = millis();
    if (tiempoActual - tiempoAnterior >= intervaloServo) {
      tiempoAnterior = tiempoActual;

      servo.write(angulo);

      if (subiendo) angulo += 5;
      else angulo -= 5;

      if (angulo >= 180) subiendo = false;
      if (angulo <= 0) subiendo = true;
    }
  } else {
    // Mantener el ángulo manual un momento antes de volver a automático
    // Por ejemplo, 3 segundos
    static unsigned long tiempoManual = 0;
    if (tiempoManual == 0) tiempoManual = millis();
    if (millis() - tiempoManual > 3000) {
      modoManual = false;
      tiempoManual = 0;
    }
  }
}
