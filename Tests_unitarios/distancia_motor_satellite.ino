#include <Servo.h>
Servo servo;
int TRIG = 10;
int ECO = 9;
int LED = 3;
// Mínimo cambio: usar tipo adecuado para pulseIn
unsigned long DURACION;
float DISTANCIA;
int angulo = 0;

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);    // <-- CAMBIO: ECO debe ser INPUT
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW); // asegurar TRIG en LOW al inicio
  servo.attach (13);
  Serial.begin(9600);
  
}

void loop() {
  // Generar pulso de trigger correcto (10 microsegundos)
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);  // <-- CAMBIO: usar microsegundos, no delay(1)
  digitalWrite(TRIG, LOW);

  // Leer duración (añadido timeout para no bloquear)
  DURACION = pulseIn(ECO, HIGH, 30000UL); // timeout 30 ms

  if (DURACION == 0) {
    Serial.println("No echo (timeout)");
    digitalWrite(LED, LOW);
  } else {
    DISTANCIA = DURACION / 58.2; // conversión a cm
    Serial.print("Dur (us): ");
    Serial.print(DURACION);
    Serial.print("  Dist (cm): ");
    Serial.println(DISTANCIA);

    if (DISTANCIA < 20.0) digitalWrite(LED, HIGH);
    else digitalWrite(LED, LOW);
  }

  if (Serial.available() > 0) {
    int comando = Serial.parseInt();  // leer un número enviado
    if (comando >= 0 && comando <= 180) {
      servo.write(comando);          // mover servo al ángulo deseado
      Serial.print("Servo movido a: ");
      Serial.println(comando);
    }
  }
  delay(100);
  // Mover de 0° a 180°
  for (int angulo = 0; angulo <= 180; angulo += 10) {
    servo.write(angulo);
    delay(300);             
  }

  // Regresar de 180° a 0°
  for (int angulo = 180; angulo >= 0; angulo -= 10) {
    servo.write(angulo);
    delay(300);
  }

  delay(200);
}
