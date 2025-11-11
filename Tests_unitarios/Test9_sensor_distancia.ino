int TRIG = 10;
int ECO = 9;
int LED = 3;
// Mínimo cambio: usar tipo adecuado para pulseIn
unsigned long DURACION;
float DISTANCIA;

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECO, INPUT);    // <-- CAMBIO: ECO debe ser INPUT
  pinMode(LED, OUTPUT);
  digitalWrite(TRIG, LOW); // asegurar TRIG en LOW al inicio
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

  delay(200);
}
